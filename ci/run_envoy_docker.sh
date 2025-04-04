#!/bin/bash

set -e

# shellcheck source=ci/envoy_build_sha.sh
. "$(dirname "$0")"/envoy_build_sha.sh

function is_windows() {
  [[ "$(uname -s)" == *NT* ]]
}

read -ra ENVOY_DOCKER_OPTIONS <<< "${ENVOY_DOCKER_OPTIONS:-}"

# TODO(phlax): uppercase these env vars
export HTTP_PROXY="${http_proxy:-}"
export HTTPS_PROXY="${https_proxy:-}"
export NO_PROXY="${no_proxy:-}"
export GOPROXY="${go_proxy:-}"

if is_windows; then
  [[ -z "${IMAGE_NAME}" ]] && IMAGE_NAME="envoyproxy/envoy-build-windows2019"
  # TODO(sunjayBhatia): Currently ENVOY_DOCKER_OPTIONS is ignored on Windows because
  # CI sets it to a Linux-specific value. Undo this once https://github.com/envoyproxy/envoy/issues/13272
  # is resolved.
  ENVOY_DOCKER_OPTIONS=()
  # Replace MSYS style drive letter (/c/) with Windows drive letter designation (C:/)
  DEFAULT_ENVOY_DOCKER_BUILD_DIR=$(echo "${TEMP}" | sed -E "s#^/([a-zA-Z])/#\1:/#")/envoy-docker-build
  BUILD_DIR_MOUNT_DEST=C:/build
  SOURCE_DIR=$(echo "${PWD}" | sed -E "s#^/([a-zA-Z])/#\1:/#")
  SOURCE_DIR_MOUNT_DEST=C:/source
  START_COMMAND=("bash" "-c" "cd /c/source && export HOME=/c/build && $*")
else
  [[ -z "${IMAGE_NAME}" ]] && IMAGE_NAME="envoyproxy/envoy-build-ubuntu"
  # We run as root and later drop permissions. This is required to setup the USER
  # in useradd below, which is need for correct Python execution in the Docker
  # environment.
  ENVOY_DOCKER_OPTIONS+=(-u root:root)
  DOCKER_USER_ARGS=()
  DOCKER_GROUP_ARGS=()
  DEFAULT_ENVOY_DOCKER_BUILD_DIR=/tmp/envoy-docker-build
  USER_UID="$(id -u)"
  USER_GID="$(id -g)"
  if [[ -n "$ENVOY_DOCKER_IN_DOCKER" ]]; then
      ENVOY_DOCKER_OPTIONS+=(-v /var/run/docker.sock:/var/run/docker.sock)
      DOCKER_GID="$(stat -c %g /var/run/docker.sock 2>/dev/null || stat -f %g /var/run/docker.sock)"
      DOCKER_USER_ARGS=(--gid "${DOCKER_GID}")
      DOCKER_GROUP_ARGS=(--gid "${DOCKER_GID}")
  else
      DOCKER_GROUP_ARGS+=(--gid "${USER_GID}")
      DOCKER_USER_ARGS+=(--gid "${USER_GID}")
  fi
  BUILD_DIR_MOUNT_DEST=/build
  SOURCE_DIR="${PWD}"
  SOURCE_DIR_MOUNT_DEST=/source
  START_COMMAND=(
      "/bin/bash"
      "-lc"
      "groupadd ${DOCKER_GROUP_ARGS[*]} -f envoygroup \
          && useradd -o --uid ${USER_UID} ${DOCKER_USER_ARGS[*]} --no-create-home --home-dir /build envoybuild \
          && usermod -a -G pcap envoybuild \
          && chown envoybuild:envoygroup /build \
          && chown envoybuild /proc/self/fd/2 \
	  && sudo apt-get update -o Acquire::AllowInsecureRepositories=true   -o Acquire::AllowDowngradeToInsecureRepositories=true \
	  && sudo apt-get install cmake ninja-build -y \
          && sudo -EHs -u envoybuild bash -c 'cd /source && $*'")
fi

# The IMAGE_ID defaults to the CI hash but can be set to an arbitrary image ID (found with 'docker
# images').
[[ -z "${IMAGE_ID}" ]] && IMAGE_ID="${ENVOY_BUILD_SHA}"
[[ -z "${ENVOY_DOCKER_BUILD_DIR}" ]] && ENVOY_DOCKER_BUILD_DIR="${DEFAULT_ENVOY_DOCKER_BUILD_DIR}"
# Replace backslash with forward slash for Windows style paths
ENVOY_DOCKER_BUILD_DIR="${ENVOY_DOCKER_BUILD_DIR//\\//}"
mkdir -p "${ENVOY_DOCKER_BUILD_DIR}"

[[ -t 1 ]] && ENVOY_DOCKER_OPTIONS+=("-it")
[[ -f .git ]] && [[ ! -d .git ]] && ENVOY_DOCKER_OPTIONS+=(-v "$(git rev-parse --git-common-dir):$(git rev-parse --git-common-dir)")
[[ -n "${SSH_AUTH_SOCK}" ]] && ENVOY_DOCKER_OPTIONS+=(-v "${SSH_AUTH_SOCK}:${SSH_AUTH_SOCK}" -e SSH_AUTH_SOCK)

export ENVOY_BUILD_IMAGE="${IMAGE_NAME}:${IMAGE_ID}"

VOLUMES=(
    -v "${ENVOY_DOCKER_BUILD_DIR}":"${BUILD_DIR_MOUNT_DEST}"
    -v "${SOURCE_DIR}":"${SOURCE_DIR_MOUNT_DEST}")

if ! is_windows && [[ -n "$ENVOY_DOCKER_IN_DOCKER" ]]; then
    # Create a "shared" directory that has the same path in/outside the container
    # This allows the host docker engine to see artefacts using a temporary path created inside the container,
    # at the same path.
    # For example, a directory created with `mktemp -d --tmpdir /tmp/bazel-shared` can be mounted as a volume
    # from within the build container.
    SHARED_TMP_DIR=/tmp/bazel-shared
    mkdir -p "${SHARED_TMP_DIR}"
    chmod +rwx "${SHARED_TMP_DIR}"
    VOLUMES+=(-v "${SHARED_TMP_DIR}":"${SHARED_TMP_DIR}")
fi

if [[ -n "${ENVOY_DOCKER_PULL}" ]]; then
    time docker pull "${ENVOY_BUILD_IMAGE}"
fi


# Since we specify an explicit hash, docker-run will pull from the remote repo if missing.
set -x
docker run --rm \
       "${ENVOY_DOCKER_OPTIONS[@]}" \
       "${VOLUMES[@]}" \
       -e AZP_BRANCH \
       -e AZP_COMMIT_SHA \
       -e AZP_TARGET_BRANCH \
       -e HTTP_PROXY \
       -e HTTPS_PROXY \
       -e NO_PROXY \
       -e GOPROXY \
       -e BAZEL_STARTUP_OPTIONS \
       -e BAZEL_BUILD_EXTRA_OPTIONS \
       -e BAZEL_EXTRA_TEST_OPTIONS \
       -e BAZEL_FAKE_SCM_REVISION \
       -e BAZEL_REMOTE_CACHE \
       -e DOCKERHUB_USERNAME \
       -e DOCKERHUB_PASSWORD \
       -e ENVOY_STDLIB \
       -e BUILD_REASON \
       -e BAZEL_NO_CACHE_TEST_RESULTS \
       -e BAZEL_REMOTE_INSTANCE \
       -e BAZEL_REMOTE_INSTANCE_BRANCH \
       -e GOOGLE_BES_PROJECT_ID \
       -e GCP_SERVICE_ACCOUNT_KEY \
       -e NUM_CPUS \
       -e ENVOY_BRANCH \
       -e ENVOY_RBE \
       -e ENVOY_BUILD_IMAGE \
       -e ENVOY_SRCDIR \
       -e ENVOY_BUILD_TARGET \
       -e ENVOY_BUILD_DEBUG_INFORMATION \
       -e ENVOY_BUILD_FILTER_EXAMPLE \
       -e ENVOY_COMMIT \
       -e ENVOY_HEAD_REF \
       -e ENVOY_PUBLISH_DRY_RUN \
       -e ENVOY_REPO \
       -e SYSTEM_PULLREQUEST_PULLREQUESTNUMBER \
       -e GCS_ARTIFACT_BUCKET \
       -e GITHUB_TOKEN \
       -e GITHUB_APP_ID \
       -e GITHUB_INSTALL_ID \
       -e NETLIFY_TRIGGER_URL \
       -e BUILD_SOURCEBRANCHNAME \
       -e BAZELISK_BASE_URL \
       -e ENVOY_BUILD_ARCH \
       -e SLACK_TOKEN \
       -e BUILD_URI\
       -e REPO_URI \
       -e SYSTEM_STAGEDISPLAYNAME \
       -e SYSTEM_JOBDISPLAYNAME \
       -e SYSTEM_PULLREQUEST_PULLREQUESTNUMBER \
       "${ENVOY_BUILD_IMAGE}" \
       "${START_COMMAND[@]}"
