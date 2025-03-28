#!/bin/bash

# Run a CI build/test target, e.g. docs, asan.

set -e


# TODO(phlax): Clarify and/or integrate SRCDIR and ENVOY_SRCDIR
export SRCDIR="${SRCDIR:-$PWD}"
export ENVOY_SRCDIR="${ENVOY_SRCDIR:-$PWD}"
NO_BUILD_SETUP="${NO_BUILD_SETUP:-}"

if [[ -z "$NO_BUILD_SETUP" ]]; then
    # shellcheck source=ci/setup_cache.sh
    . "$(dirname "$0")"/setup_cache.sh
    # shellcheck source=ci/build_setup.sh
    . "$(dirname "$0")"/build_setup.sh

    echo "building using ${NUM_CPUS} CPUs"
    echo "building for ${ENVOY_BUILD_ARCH}"
fi
cd "${SRCDIR}"

if [[ "${ENVOY_BUILD_ARCH}" == "x86_64" ]]; then
  BUILD_ARCH_DIR="/linux/amd64"
elif [[ "${ENVOY_BUILD_ARCH}" == "aarch64" ]]; then
  BUILD_ARCH_DIR="/linux/arm64"
else
  # Fall back to use the ENVOY_BUILD_ARCH itself.
  BUILD_ARCH_DIR="/linux/${ENVOY_BUILD_ARCH}"
fi

function collect_build_profile() {
  declare -g build_profile_count=${build_profile_count:-1}
  mv -f \
     "$(bazel info "${BAZEL_BUILD_OPTIONS[@]}" output_base)/command.profile.gz" \
     "${ENVOY_BUILD_PROFILE}/${build_profile_count}-$1.profile.gz" \
      || :
  ((build_profile_count++))
}

function bazel_with_collection() {
  local failed_logs
  declare -r BAZEL_OUTPUT="${ENVOY_SRCDIR}"/bazel.output.txt
  bazel "$@" | tee "${BAZEL_OUTPUT}"
  declare BAZEL_STATUS="${PIPESTATUS[0]}"
  if [ "${BAZEL_STATUS}" != "0" ]
  then
    pushd bazel-testlogs
    failed_logs=$(grep "  /build.*test.log" "${BAZEL_OUTPUT}" | sed -e 's/  \/build.*\/testlogs\/\(.*\)/\1/')
    while read -r f; do
      cp --parents -f "$f" "${ENVOY_FAILED_TEST_LOGS}"
    done <<< "$failed_logs"
    popd
    exit "${BAZEL_STATUS}"
  fi
  collect_build_profile "$1"
}

function cp_binary_for_image_build() {
  local BINARY_TYPE="$1"
  local COMPILE_TYPE="$2"
  local EXE_NAME="$3"

  # TODO(mattklein123): Replace this with caching and a different job which creates images.
  local BASE_TARGET_DIR="${ENVOY_SRCDIR}${BUILD_ARCH_DIR}"
  local TARGET_DIR=build_"${EXE_NAME}"_"${BINARY_TYPE}"
  local FINAL_DELIVERY_DIR="${ENVOY_DELIVERY_DIR}"/"${EXE_NAME}"

  echo "Copying binary for image build..."
  mkdir -p "${BASE_TARGET_DIR}"/"${TARGET_DIR}"
  cp -f "${FINAL_DELIVERY_DIR}"/envoy "${BASE_TARGET_DIR}"/"${TARGET_DIR}"
  if [[ "${COMPILE_TYPE}" == "dbg" || "${COMPILE_TYPE}" == "opt" ]]; then
    cp -f "${FINAL_DELIVERY_DIR}"/envoy.dwp "${BASE_TARGET_DIR}"/"${TARGET_DIR}"
  fi

  # Tools for the tools image. Strip to save size.
  strip bazel-bin/test/tools/schema_validator/schema_validator_tool \
    -o "${BASE_TARGET_DIR}"/"${TARGET_DIR}"/schema_validator_tool

  # Copy the su-exec utility binary into the image
  cp -f bazel-bin/external/com_github_ncopa_suexec/su-exec "${BASE_TARGET_DIR}"/"${TARGET_DIR}"

  # Stripped binaries for the debug image.
  mkdir -p "${BASE_TARGET_DIR}"/"${TARGET_DIR}"_stripped
  strip "${FINAL_DELIVERY_DIR}"/envoy -o "${BASE_TARGET_DIR}"/"${TARGET_DIR}"_stripped/envoy

  # only if BUILD_REASON exists (running in AZP)
  if [[ "${BUILD_REASON}" ]]; then
    # Copy for azp which doesn't preserve permissions
    tar czf "${ENVOY_BUILD_DIR}"/"${EXE_NAME}"_binary.tar.gz -C "${BASE_TARGET_DIR}" "${TARGET_DIR}" "${TARGET_DIR}"_stripped

    # Remove binaries to save space
    rm -rf "${BASE_TARGET_DIR:?}"/"${TARGET_DIR}" "${BASE_TARGET_DIR:?}"/"${TARGET_DIR}"_stripped "${FINAL_DELIVERY_DIR:?}"/envoy{,.dwp} \
      bazel-bin/"${ENVOY_BIN}"{,.dwp}
  fi
}

function bazel_binary_build() {
  local BINARY_TYPE="$1"
  if [[ "${BINARY_TYPE}" == "release" ]]; then
    COMPILE_TYPE="opt"
  elif [[ "${BINARY_TYPE}" == "debug" ]]; then
    COMPILE_TYPE="dbg"
  elif [[ "${BINARY_TYPE}" == "sizeopt" ]]; then
    # The COMPILE_TYPE variable is redundant in this case and is only here for
    # readability. It is already set in the .bazelrc config for sizeopt.
    COMPILE_TYPE="opt"
    CONFIG_ARGS="--config=sizeopt"
  elif [[ "${BINARY_TYPE}" == "fastbuild" ]]; then
    COMPILE_TYPE="fastbuild"
  fi

  local BUILD_TARGET="$2"
  local BUILD_DEBUG_INFORMATION="$3"
  local EXE_NAME="$4"
  local FINAL_DELIVERY_DIR="${ENVOY_DELIVERY_DIR}"/"${EXE_NAME}"
  mkdir -p "${FINAL_DELIVERY_DIR}"

  echo "Building (type=${BINARY_TYPE} target=${BUILD_TARGET} debug=${BUILD_DEBUG_INFORMATION} name=${EXE_NAME})..."
  ENVOY_BIN=$(echo "${BUILD_TARGET}" | sed -e 's#^@\([^/]*\)/#external/\1#;s#^//##;s#:#/#')
  echo "ENVOY_BIN=${ENVOY_BIN}"

  # This is a workaround for https://github.com/bazelbuild/bazel/issues/11834
  [[ -n "${ENVOY_RBE}" ]] && rm -rf bazel-bin/"${ENVOY_BIN}"*

  bazel build "${BAZEL_BUILD_OPTIONS[@]}" --remote_download_toplevel -c "${COMPILE_TYPE}" "${BUILD_TARGET}" ${CONFIG_ARGS}
  collect_build_profile "${BINARY_TYPE}"_build

  # Copy the built envoy binary somewhere that we can access outside of the
  # container.
  cp -f bazel-bin/"${ENVOY_BIN}" "${FINAL_DELIVERY_DIR}"/envoy

  if [[ "${COMPILE_TYPE}" == "dbg" || "${COMPILE_TYPE}" == "opt" ]]; then
    # Generate dwp file for debugging since we used split DWARF to reduce binary
    # size
    bazel build "${BAZEL_BUILD_OPTIONS[@]}" --remote_download_toplevel -c "${COMPILE_TYPE}" "${BUILD_DEBUG_INFORMATION}" ${CONFIG_ARGS}
    # Copy the debug information
    cp -f bazel-bin/"${ENVOY_BIN}".dwp "${FINAL_DELIVERY_DIR}"/envoy.dwp
  fi

  # Validation tools for the tools image.
  bazel build "${BAZEL_BUILD_OPTIONS[@]}" --remote_download_toplevel -c "${COMPILE_TYPE}" \
    //test/tools/schema_validator:schema_validator_tool ${CONFIG_ARGS}

  # Build su-exec utility
  bazel build "${BAZEL_BUILD_OPTIONS[@]}" --remote_download_toplevel -c "${COMPILE_TYPE}" external:su-exec
  cp_binary_for_image_build "${BINARY_TYPE}" "${COMPILE_TYPE}" "${EXE_NAME}"
}

function bazel_envoy_binary_build() {
  bazel_binary_build "$1" "${ENVOY_BUILD_TARGET}" "${ENVOY_BUILD_DEBUG_INFORMATION}" envoy
}

function bazel_contrib_binary_build() {
  bazel_binary_build "$1" "${ENVOY_CONTRIB_BUILD_TARGET}" "${ENVOY_CONTRIB_BUILD_DEBUG_INFORMATION}" envoy-contrib
}

function run_ci_verify () {
    export DOCKER_NO_PULL=1
    export DOCKER_RMI_CLEANUP=1
    # This is set to simulate an environment where users have shared home drives protected
    # by a strong umask (ie only group readable by default).
    umask 027
    chmod -R o-rwx examples/
    "${ENVOY_SRCDIR}/ci/verify_examples.sh" "${@}"
}

CI_TARGET=$1
shift

if [[ $# -ge 1 ]]; then
  COVERAGE_TEST_TARGETS=("$@")
  TEST_TARGETS=("$@")
else
  # Coverage test will add QUICHE tests by itself.
  COVERAGE_TEST_TARGETS=("//test/...")
  if [[ "$CI_TARGET" == "bazel.release" ]]; then
    # We test contrib on release only.
    COVERAGE_TEST_TARGETS=("${COVERAGE_TEST_TARGETS[@]}" "//contrib/...")
  elif [[ "${CI_TARGET}" == "bazel.msan" ]]; then
    COVERAGE_TEST_TARGETS=("${COVERAGE_TEST_TARGETS[@]}" "-//test/extensions/...")
  fi
  TEST_TARGETS=("${COVERAGE_TEST_TARGETS[@]}" "@com_github_google_quiche//:ci_tests")
fi

if [[ "$CI_TARGET" =~ bazel.* ]]; then
    CI_TARGET="$(echo "${CI_TARGET}" | cut -d. -f2-)"
fi

case $CI_TARGET in
    api)
        # Use libstdc++ because the API booster links to prebuilt libclang*/libLLVM* installed in /opt/llvm/lib,
        # which is built with libstdc++. Using libstdc++ for whole of the API CI job to avoid unnecessary rebuild.
        ENVOY_STDLIB="libstdc++"
        setup_clang_toolchain
        export LLVM_CONFIG="${LLVM_ROOT}"/bin/llvm-config
	bazel build "${BAZEL_BUILD_OPTIONS[@]}" -c fastbuild //api/meta_protocol_proxy/v1alpha:pkg_go_proto && \
	bazel build "${BAZEL_BUILD_OPTIONS[@]}" -c fastbuild //api/meta_protocol_proxy/admin/v1alpha:pkg_go_proto && \
	bazel build "${BAZEL_BUILD_OPTIONS[@]}" -c fastbuild //api/meta_protocol_proxy/filters/router/v1alpha:pkg_go_proto && \
	bazel build "${BAZEL_BUILD_OPTIONS[@]}" -c fastbuild //api/meta_protocol_proxy/filters/local_ratelimit/v1alpha:pkg_go_proto && \
	bazel build "${BAZEL_BUILD_OPTIONS[@]}" -c fastbuild //api/meta_protocol_proxy/filters/global_ratelimit/v1alpha:pkg_go_proto && \
	bazel build "${BAZEL_BUILD_OPTIONS[@]}" -c fastbuild //api/meta_protocol_proxy/filters/metadata_exchange/v1alpha:pkg_go_proto && \
	bazel build "${BAZEL_BUILD_OPTIONS[@]}" -c fastbuild //api/meta_protocol_proxy/filters/istio_stats/v1alpha:pkg_go_proto && \
	bazel build "${BAZEL_BUILD_OPTIONS[@]}" -c fastbuild //api/meta_protocol_proxy/config/route/v1alpha:pkg_go_proto
        ;;

    api_compat)
        echo "Checking API for breaking changes to protobuf backwards compatibility..."
        BASE_BRANCH_REF=$("${ENVOY_SRCDIR}"/tools/git/last_github_commit.sh)
        COMMIT_TITLE=$(git log -n 1 --pretty='format:%C(auto)%h (%s, %ad)' "${BASE_BRANCH_REF}")
        echo -e "\tUsing base commit ${COMMIT_TITLE}"
        # BAZEL_BUILD_OPTIONS needed for setting the repository_cache param.
        bazel run "${BAZEL_BUILD_OPTIONS[@]}" \
              //tools/api_proto_breaking_change_detector:detector_ci \
              "${BASE_BRANCH_REF}"
        ;;

    asan)
        setup_clang_toolchain
        BAZEL_BUILD_OPTIONS+=(
            -c dbg
            "--config=clang-asan"
            "--build_tests_only"
            "--remote_download_minimal")
        echo "bazel ASAN/UBSAN debug build with tests"
        echo "Building and testing envoy tests ${TEST_TARGETS[*]}"
        bazel_with_collection test "${BAZEL_BUILD_OPTIONS[@]}" "${TEST_TARGETS[@]}"
        if [ "${ENVOY_BUILD_FILTER_EXAMPLE}" == "1" ]; then
            echo "Building and testing envoy-filter-example tests..."
            pushd "${ENVOY_FILTER_EXAMPLE_SRCDIR}"
            bazel_with_collection \
                test "${BAZEL_BUILD_OPTIONS[@]}" \
                "${ENVOY_FILTER_EXAMPLE_TESTS[@]}"
            popd
        fi
        # TODO(mattklein123): This part of the test is now flaky in CI and it's unclear why, possibly
        # due to sandboxing issue. Debug and enable it again.
        # if [ "${CI_SKIP_INTEGRATION_TEST_TRAFFIC_TAPPING}" != "1" ] ; then
        # Also validate that integration test traffic tapping (useful when debugging etc.)
        # works. This requires that we set TAP_PATH. We do this under bazel.asan to
        # ensure a debug build in CI.
        # echo "Validating integration test traffic tapping..."
        # bazel_with_collection test "${BAZEL_BUILD_OPTIONS[@]}" \
            #   --run_under=@envoy//bazel/test:verify_tap_test.sh \
            #   //test/extensions/transport_sockets/tls/integration:ssl_integration_test
        # fi
        ;;

    check_and_fix_proto_format)
        setup_clang_toolchain
        echo "Check and fix proto format ..."
        "${ENVOY_SRCDIR}/ci/check_and_fix_format.sh"
        ;;

    check_proto_format)
        setup_clang_toolchain
        echo "Check proto format ..."
        "${ENVOY_SRCDIR}/tools/proto_format/proto_format.sh" check
        ;;

    clang_tidy)
        # clang-tidy will warn on standard library issues with libc++
        ENVOY_STDLIB="libstdc++"
        setup_clang_toolchain
        export CLANG_TIDY_FIX_DIFF="${ENVOY_TEST_TMPDIR}/lint-fixes/clang-tidy-fixed.diff"
        export FIX_YAML="${ENVOY_TEST_TMPDIR}/lint-fixes/clang-tidy-fixes.yaml"
        export CLANG_TIDY_APPLY_FIXES=1
        mkdir -p "${ENVOY_TEST_TMPDIR}/lint-fixes"
        BAZEL_BUILD_OPTIONS+=(--remote_download_minimal)
        NUM_CPUS=$NUM_CPUS "${ENVOY_SRCDIR}"/ci/run_clang_tidy.sh "$@" || {
            if [[ -s "$FIX_YAML" ]]; then
                echo >&2
                echo "Diff/yaml files with (some) fixes will be uploaded. Please check the artefacts for this PR run in the azure pipeline." >&2
                echo >&2
            else
                echo "Clang-tidy failed." >&2
            fi
            exit 1
        }
        ;;

    compile_time_options)
        # Right now, none of the available compile-time options conflict with each other. If this
        # changes, this build type may need to be broken up.
        COMPILE_TIME_OPTIONS=(
            "--define" "admin_html=disabled"
            "--define" "signal_trace=disabled"
            "--define" "hot_restart=disabled"
            "--define" "google_grpc=disabled"
            "--define" "boringssl=fips"
            "--define" "log_debug_assert_in_release=enabled"
            "--define" "path_normalization_by_default=true"
            "--define" "deprecated_features=disabled"
            "--define" "tcmalloc=gperftools"
            "--define" "zlib=ng"
            "--define" "uhv=enabled"
            "--@envoy//bazel:http3=False"
            "--@envoy//source/extensions/filters/http/kill_request:enabled"
            "--test_env=ENVOY_HAS_EXTRA_EXTENSIONS=true"
            "--remote_download_minimal")
        ENVOY_STDLIB="${ENVOY_STDLIB:-libstdc++}"
        setup_clang_toolchain
        # This doesn't go into CI but is available for developer convenience.
        echo "bazel with different compiletime options build with tests..."
        cd "${ENVOY_FILTER_EXAMPLE_SRCDIR}"
        TEST_TARGETS=("${TEST_TARGETS[@]/#\/\//@envoy\/\/}")
        # Building all the dependencies from scratch to link them against libc++.
        echo "Building and testing with wasm=wamr: ${TEST_TARGETS[*]}"
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
            --define wasm=wamr \
            "${COMPILE_TIME_OPTIONS[@]}" \
            -c fastbuild \
            "${TEST_TARGETS[@]}" \
            --test_tag_filters=-nofips \
            --build_tests_only
        echo "Building and testing with wasm=wasmtime: and admin_functionality and admin_html disabled ${TEST_TARGETS[*]}"
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
            --define wasm=wasmtime \
            --define admin_html=disabled \
            --define admin_functionality=disabled \
            "${COMPILE_TIME_OPTIONS[@]}" \
            -c fastbuild \
            "${TEST_TARGETS[@]}" \
            --test_tag_filters=-nofips \
            --build_tests_only
        echo "Building and testing with wasm=wavm: ${TEST_TARGETS[*]}"
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
            --define wasm=wavm \
            "${COMPILE_TIME_OPTIONS[@]}" \
            -c fastbuild \
            "${TEST_TARGETS[@]}" \
            --test_tag_filters=-nofips \
            --build_tests_only
        # "--define log_debug_assert_in_release=enabled" must be tested with a release build, so run only
        # these tests under "-c opt" to save time in CI.
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
            --define wasm=wavm \
            "${COMPILE_TIME_OPTIONS[@]}" \
            -c opt \
            @envoy//test/common/common:assert_test \
            @envoy//test/server:server_test
        # "--define log_fast_debug_assert_in_release=enabled" must be tested with a release build, so run only these tests under "-c opt" to save time in CI. This option will test only ASSERT()s without SLOW_ASSERT()s, so additionally disable "--define log_debug_assert_in_release" which compiles in both.
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
            --define wasm=wavm \
            "${COMPILE_TIME_OPTIONS[@]}" \
            -c opt \
            @envoy//test/common/common:assert_test \
            --define log_fast_debug_assert_in_release=enabled \
            --define log_debug_assert_in_release=disabled
        echo "Building binary with wasm=wavm... and logging disabled"
        bazel build "${BAZEL_BUILD_OPTIONS[@]}" \
              --define wasm=wavm \
              --define enable_logging=disabled \
              "${COMPILE_TIME_OPTIONS[@]}" \
              -c fastbuild \
              @envoy//source/exe:envoy-static \
              --build_tag_filters=-nofips
        collect_build_profile build
        ;;

    coverage|fuzz_coverage)
        setup_clang_toolchain
        echo "${CI_TARGET} build with tests ${COVERAGE_TEST_TARGETS[*]}"
        if [[ "$CI_TARGET" == "fuzz_coverage" ]]; then
            export FUZZ_COVERAGE=true
        fi
        # We use custom BAZEL_BUILD_OPTIONS here to cover profiler's code.
        BAZEL_BUILD_OPTION_LIST="${BAZEL_BUILD_OPTIONS[*]} --define tcmalloc=gperftools" \
            "${ENVOY_SRCDIR}/test/run_envoy_bazel_coverage.sh" \
            "${COVERAGE_TEST_TARGETS[@]}"
        collect_build_profile coverage
        ;;

    coverage-upload|fuzz_coverage-upload)
        setup_clang_toolchain
        if [[ "$CI_TARGET" == "fuzz_coverage-upload" ]]; then
            TARGET=fuzz_coverage
        else
            TARGET=coverage
        fi
        "${ENVOY_SRCDIR}/ci/upload_gcs_artifact.sh" "/source/generated/${TARGET}" "$TARGET"
        ;;

    debug)
        setup_clang_toolchain
        echo "Testing ${TEST_TARGETS[*]}"
        # Make sure that there are no regressions to building Envoy with autolink disabled.
        EXTRA_OPTIONS=(
            "--define" "library_autolink=disabled")
        bazel test "${BAZEL_BUILD_OPTIONS[@]}" \
              "${EXTRA_OPTIONS[@]}" \
              -c dbg \
              "${TEST_TARGETS[@]}"
        echo "bazel debug build with tests..."
        bazel_envoy_binary_build debug
        ;;

    debug.server_only)
        setup_clang_toolchain
        echo "bazel debug build..."
        bazel_envoy_binary_build debug
        ;;

    deps)
        setup_clang_toolchain
        echo "dependency validate_test..."
        bazel run "${BAZEL_BUILD_OPTIONS[@]}" \
              //tools/dependency:validate_test
        echo "verifying dependencies..."
        # Validate dependency relationships between core/extensions and external deps.
        time bazel run "${BAZEL_BUILD_OPTIONS[@]}" \
             //tools/dependency:validate
        # Validate repository metadata.
        echo "check repositories..."
        "${ENVOY_SRCDIR}/tools/check_repositories.sh"
        echo "check dependencies..."
        # Using todays date as an action_env expires the NIST cache daily, which is the update frequency
        TODAY_DATE=$(date -u -I"date")
        export TODAY_DATE
        bazel run "${BAZEL_BUILD_OPTIONS[@]}" //tools/dependency:check \
              --action_env=TODAY_DATE \
              -- -v warn \
                 -c cves release_dates releases
        # Run dependabot tests
        echo "Check dependabot ..."
        bazel run "${BAZEL_BUILD_OPTIONS[@]}" \
              //tools/dependency:dependatool
        ;;

    dev)
        setup_clang_toolchain
        bazel_envoy_binary_build fastbuild	
        ;;

    dev.contrib)
        setup_clang_toolchain
        # This doesn't go into CI but is available for developer convenience.
        echo "bazel fastbuild build with contrib extensions and tests..."
        echo "Building..."
        bazel_contrib_binary_build fastbuild
        echo "Testing ${TEST_TARGETS[*]}"
        bazel test "${BAZEL_BUILD_OPTIONS[@]}" \
              -c fastbuild \
              "${TEST_TARGETS[@]}"
        ;;

    distribution)
        echo "Building distro packages..."
        setup_clang_toolchain
        # Extract the Envoy binary from the tarball
        mkdir -p distribution/custom
        if [[ "${ENVOY_BUILD_ARCH}" == "x86_64" ]]; then
            ENVOY_RELEASE_TARBALL="/build/bazel.release/x64/bin/release.tar.zst"
        else
            ENVOY_RELEASE_TARBALL="/build/bazel.release/arm64/bin/release.tar.zst"
        fi
        bazel run "${BAZEL_BUILD_OPTIONS[@]}" \
              //tools/zstd \
              -- --stdout \
                 -d "$ENVOY_RELEASE_TARBALL" \
            | tar xfO - envoy > distribution/custom/envoy
        # Build the packages
        bazel build "${BAZEL_BUILD_OPTIONS[@]}" \
              --remote_download_toplevel \
              -c opt \
              --//distribution:envoy-binary=//distribution:custom/envoy \
              //distribution:packages.tar.gz
        if [[ "${ENVOY_BUILD_ARCH}" == "x86_64" ]]; then
            cp -a bazel-bin/distribution/packages.tar.gz "${ENVOY_BUILD_DIR}/packages.x64.tar.gz"
        else
            cp -a bazel-bin/distribution/packages.tar.gz "${ENVOY_BUILD_DIR}/packages.arm64.tar.gz"
        fi
        ;;

    docs)
        setup_clang_toolchain
        echo "generating docs..."
        # Build docs.
        "${ENVOY_SRCDIR}/docs/build.sh"
        ;;

    docs-upload)
        setup_clang_toolchain
        "${ENVOY_SRCDIR}/ci/upload_gcs_artifact.sh" /source/generated/docs docs
        ;;

    docs-publish-latest)
        BUILD_SHA=$(git rev-parse HEAD)
        curl -X POST -d "$BUILD_SHA" "$NETLIFY_TRIGGER_URL"
        ;;

    docker-upload)
        setup_clang_toolchain
        "${ENVOY_SRCDIR}/ci/upload_gcs_artifact.sh" "${BUILD_DIR}/build_images" docker
        ;;

    dockerhub-publish)
        setup_clang_toolchain
        bazel run "${BAZEL_BUILD_OPTIONS[@]}" \
              //tools/distribution:update_dockerhub_repository
        ;;

    dockerhub-readme)
        setup_clang_toolchain
        bazel build "${BAZEL_BUILD_OPTIONS[@]}" \
              //distribution/dockerhub:readme
        cat bazel-bin/distribution/dockerhub/readme.md
        ;;

    fix_proto_format)
        # proto_format.sh needs to build protobuf.
        setup_clang_toolchain
        "${ENVOY_SRCDIR}/tools/proto_format/proto_format.sh" fix
        ;;

    format)
        setup_clang_toolchain
        "${ENVOY_SRCDIR}/ci/format_pre.sh"
        ;;

    fuzz)
        setup_clang_toolchain
        FUZZ_TEST_TARGETS=("$(bazel query "${BAZEL_GLOBAL_OPTIONS[@]}" "attr('tags','fuzzer',${TEST_TARGETS[*]})")")
        echo "bazel ASAN libFuzzer build with fuzz tests ${FUZZ_TEST_TARGETS[*]}"
        echo "Building envoy fuzzers and executing 100 fuzz iterations..."
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
            --config=asan-fuzzer \
            "${FUZZ_TEST_TARGETS[@]}" \
            --test_arg="-runs=10"
        ;;

    gcc)
        BAZEL_BUILD_OPTIONS+=("--test_env=HEAPCHECK=")
        setup_gcc_toolchain
        echo "Testing ${TEST_TARGETS[*]}"
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
            -c fastbuild  \
            --remote_download_minimal \
            -- "${TEST_TARGETS[@]}"
        echo "bazel release build with gcc..."
        bazel_envoy_binary_build fastbuild
        ;;

    msan)
        ENVOY_STDLIB=libc++
        setup_clang_toolchain
        # rbe-toolchain-msan must comes as first to win library link order.
        BAZEL_BUILD_OPTIONS=(
            "--config=rbe-toolchain-msan"
            "${BAZEL_BUILD_OPTIONS[@]}"
            "-c" "dbg"
            "--build_tests_only"
            "--remote_download_minimal")
        echo "bazel MSAN debug build with tests"
        echo "Building and testing envoy tests ${TEST_TARGETS[*]}"
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
            -- "${TEST_TARGETS[@]}"
        ;;

    publish)
        setup_clang_toolchain
        BUILD_SHA="$(git rev-parse HEAD)"
        VERSION_DEV="$(cut -d- -f2 < VERSION.txt)"
        PUBLISH_ARGS=(
            --publish-commitish="$BUILD_SHA"
            --publish-assets=/build/release.signed/release.signed.tar.zst)
        if [[ "$VERSION_DEV" == "dev" ]] || [[ -n "$ENVOY_PUBLISH_DRY_RUN" ]]; then
            PUBLISH_ARGS+=(--dry-run)
        fi
        bazel run "${BAZEL_BUILD_OPTIONS[@]}" \
              @envoy_repo//:publish \
              -- "${PUBLISH_ARGS[@]}"
        ;;

    release)
        # When testing memory consumption, we want to test against exact byte-counts
        # where possible. As these differ between platforms and compile options, we
        # define the 'release' builds as canonical and test them only in CI, so the
        # toolchain is kept consistent. This ifdef is checked in
        # test/common/stats/stat_test_utility.cc when computing
        # Stats::TestUtil::MemoryTest::mode().
        if [[ "${ENVOY_BUILD_ARCH}" == "x86_64" ]]; then
            BAZEL_BUILD_OPTIONS+=("--test_env=ENVOY_MEMORY_TEST_EXACT=true")
        fi
        setup_clang_toolchain
        ENVOY_BINARY_DIR="${ENVOY_BUILD_DIR}/bin"
        mkdir -p "$ENVOY_BINARY_DIR"
        # As the binary build package enforces compiler options, adding here to ensure the tests and distribution build
        # reuse settings and any already compiled artefacts, the bundle itself will always be compiled
        # `--stripopt=--strip-all -c opt`
        BAZEL_RELEASE_OPTIONS=(
            --stripopt=--strip-all
            -c opt)
        # Run release tests
        echo "Testing with:"
        echo "  targets: ${TEST_TARGETS[*]}"
        echo "  build options: ${BAZEL_BUILD_OPTIONS[*]}"
        echo "  release options:  ${BAZEL_RELEASE_OPTIONS[*]}"
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
            --remote_download_minimal \
            "${BAZEL_RELEASE_OPTIONS[@]}" \
            "${TEST_TARGETS[@]}"
        # Build release binaries
        bazel build "${BAZEL_BUILD_OPTIONS[@]}" "${BAZEL_RELEASE_OPTIONS[@]}" \
              //distribution/binary:release
        # Copy release binaries to binary export directory
        cp -a \
           "bazel-bin/distribution/binary/release.tar.zst" \
           "${ENVOY_BINARY_DIR}/release.tar.zst"
        # Grab the schema_validator_tool
        # TODO(phlax): bundle this with the release when #26390 is resolved
        bazel build "${BAZEL_BUILD_OPTIONS[@]}" "${BAZEL_RELEASE_OPTIONS[@]}" \
              --remote_download_toplevel \
              //test/tools/schema_validator:schema_validator_tool.stripped
        # Copy schema_validator_tool to binary export directory
        cp -a \
           bazel-bin/test/tools/schema_validator/schema_validator_tool.stripped \
           "${ENVOY_BINARY_DIR}/schema_validator_tool"
        ;;

    release.server_only)
        setup_clang_toolchain
        echo "bazel release build..."
        bazel_envoy_binary_build release
        ;;

    release.signed)
        echo "Signing binary packages..."
        setup_clang_toolchain
        # The default config expects these files
        mkdir -p distribution/custom
        cp -a /build/bazel.*/*64 distribution/custom/
        bazel build "${BAZEL_BUILD_OPTIONS[@]}" //distribution:signed
        cp -a bazel-bin/distribution/release.signed.tar.zst "${BUILD_DIR}/envoy/"
        ;;

    sizeopt)
        setup_clang_toolchain
        echo "Testing ${TEST_TARGETS[*]}"
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
            --config=sizeopt \
            "${TEST_TARGETS[@]}"
        echo "bazel size optimized build with tests..."
        bazel_envoy_binary_build sizeopt
        ;;

    sizeopt.server_only)
        setup_clang_toolchain
        echo "bazel size optimized build..."
        bazel_envoy_binary_build sizeopt
        ;;

    tsan)
        setup_clang_toolchain
        echo "bazel TSAN debug build with tests"
        echo "Building and testing envoy tests ${TEST_TARGETS[*]}"
        bazel_with_collection \
            test "${BAZEL_BUILD_OPTIONS[@]}" \
             --config=rbe-toolchain-tsan \
             -c dbg \
             --build_tests_only \
             --remote_download_minimal \
             "${TEST_TARGETS[@]}"
        if [ "${ENVOY_BUILD_FILTER_EXAMPLE}" == "1" ]; then
            echo "Building and testing envoy-filter-example tests..."
            pushd "${ENVOY_FILTER_EXAMPLE_SRCDIR}"
            bazel_with_collection \
                test "${BAZEL_BUILD_OPTIONS[@]}" \
                -c dbg \
                --config=clang-tsan \
                "${ENVOY_FILTER_EXAMPLE_TESTS[@]}"
            popd
        fi
        ;;

    verify_distro)
        if [[ "${ENVOY_BUILD_ARCH}" == "x86_64" ]]; then
            PACKAGE_BUILD=/build/bazel.distribution/x64/packages.x64.tar.gz
        else
            PACKAGE_BUILD=/build/bazel.distribution/arm64/packages.arm64.tar.gz
        fi
        bazel run "${BAZEL_BUILD_OPTIONS[@]}" \
              //distribution:verify_packages \
              "$PACKAGE_BUILD"
        ;;

    verify_examples)
        run_ci_verify "*" "win32-front-proxy|shared"
        ;;

    verify.trigger)
        setup_clang_toolchain
        WORKFLOW="envoy-verify.yml"
        # * Note on vars *
        # `ENVOY_REPO`: Should always be envoyproxy/envoy unless testing
        # `ENVOY_BRANCH`: Target branch for PRs, source branch for others
        # `COMMIT`: This may be a merge commit in a PR
        # `ENVOY_COMMIT`: The actual last commit of branch/PR
        # `ENVOY_HEAD_REF`: must also be set in PRs to provide a unique key for job grouping,
        #   cancellation, and to discriminate from other branch CI
        COMMIT="$(git rev-parse HEAD)"
        ENVOY_COMMIT="${ENVOY_COMMIT:-${COMMIT}}"
        ENVOY_REPO="${ENVOY_REPO:-envoyproxy/envoy}"
        echo "Trigger workflow (${WORKFLOW})"
        echo "  Repo: ${ENVOY_REPO}"
        echo "  Branch: ${ENVOY_BRANCH}"
        echo "  Ref: ${COMMIT}"
        echo "  Inputs:"
        echo "    sha: ${ENVOY_COMMIT}"
        echo "    head_ref: ${ENVOY_HEAD_REF}"
        GITHUB_APP_KEY="$(echo "$GITHUB_TOKEN" | base64 -d -w0)"
        export GITHUB_APP_KEY
        INPUTS="{\"ref\":\"$COMMIT\",\"sha\":\"$ENVOY_COMMIT\",\"head_ref\":\"$ENVOY_HEAD_REF\"}"
        bazel run "${BAZEL_BUILD_OPTIONS[@]}" \
              @envoy_repo//:trigger \
              -- --repo="$ENVOY_REPO" \
                 --trigger-app-id="$GITHUB_APP_ID" \
                 --trigger-installation-id="$GITHUB_INSTALL_ID" \
                 --trigger-ref="$ENVOY_BRANCH" \
                 --trigger-workflow="$WORKFLOW" \
                 --trigger-inputs="$INPUTS"
        ;;

    *)
        echo "Invalid do_ci.sh target (${CI_TARGET}), see ci/README.md for valid targets."
        exit 1
        ;;
esac
