# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################
#
workspace(name = "meta_protocol_proxy")

# http_archive is not a native function since bazel 0.19
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "io_istio_proxy",
    strip_prefix = "proxy-1.18.1",
    sha256 = "ce682bf4bad606bfe5188e85d585a2ba12fec177254381e08da6cb27e8c872a1",
    url = "https://github.com/istio/proxy/archive/refs/tags/1.18.1.tar.gz",
)

load(
    "@io_istio_proxy//bazel:repositories.bzl",
    "docker_dependencies",
    "istioapi_dependencies",
)

istioapi_dependencies()

bind(
    name = "boringssl_crypto",
    actual = "//external:ssl",
)

# 1. Determine SHA256 `wget https://github.com/envoyproxy/envoy/archive/$COMMIT.tar.gz && sha256sum $COMMIT.tar.gz`
# 2. Update .bazelversion, envoy.bazelrc and .bazelrc if needed.
#
# Commit date: 2023-07-13
ENVOY_SHA = "5b97cffea6bf0588dc7f6efe43a9822cd376cbea"

ENVOY_SHA256 = "53a5c10fd00f6b2023c8341555d794dcd563bb1aa43944a551dc90af3c2e51d8"

ENVOY_ORG = "envoyproxy"

ENVOY_REPO = "envoy"

# To override with local envoy, just pass `--override_repository=envoy=/PATH/TO/ENVOY` to Bazel or
# persist the option in `user.bazelrc`.
http_archive(
    name = "envoy",
    sha256 = ENVOY_SHA256,
    strip_prefix = ENVOY_REPO + "-" + ENVOY_SHA,
    url = "https://github.com/" + ENVOY_ORG + "/" + ENVOY_REPO + "/archive/" + ENVOY_SHA + ".tar.gz",
    patches = [
        "//:patches/0001-expose-some-build-file-as-public.patch",
        "//:patches/0002-envoy-bazel-repo-location.patch",
        "//:patches/0003-envoy-bazel-repo-location.patch"],
)

load("@envoy//bazel:api_binding.bzl", "envoy_api_binding")

local_repository(
    name = "envoy_build_config",
    # Relative paths are also supported.
    path = "bazel/extension_config",
)

envoy_api_binding()

load("@envoy//bazel:api_repositories.bzl", "envoy_api_dependencies")

envoy_api_dependencies()

load("@envoy//bazel:repositories.bzl", "envoy_dependencies")

envoy_dependencies()

load("@envoy//bazel:repositories_extra.bzl", "envoy_dependencies_extra")

envoy_dependencies_extra()

load("@envoy//bazel:python_dependencies.bzl", "envoy_python_dependencies")

envoy_python_dependencies()

load("@base_pip3//:requirements.bzl", "install_deps")

install_deps()

load("@envoy//bazel:dependency_imports.bzl", "envoy_dependency_imports")

envoy_dependency_imports()

load("@bazel_gazelle//:deps.bzl", "go_repository")

go_repository(
    name = "org_golang_x_tools",
    importpath = "golang.org/x/tools",
    sum = "h1:po9/4sTYwOU9QPo1XTU8+YLNBPyXgS4nSdIjxNBXtxg=",
    version = "v0.1.12",
    build_external = "external",
)

# Bazel @rules_pkg

http_archive(
    name = "rules_pkg",
    sha256 = "aeca78988341a2ee1ba097641056d168320ecc51372ef7ff8e64b139516a4937",
    urls = [
        "https://github.com/bazelbuild/rules_pkg/releases/download/0.2.6-1/rules_pkg-0.2.6.tar.gz",
        "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.2.6/rules_pkg-0.2.6.tar.gz",
    ],
)

load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")

rules_pkg_dependencies()

# Docker dependencies

docker_dependencies()

load(
    "@io_bazel_rules_docker//repositories:repositories.bzl",
    container_repositories = "repositories",
)

container_repositories()

load("@io_bazel_rules_docker//repositories:deps.bzl", container_deps = "deps")

container_deps()

load(
    "@io_bazel_rules_docker//container:container.bzl",
    "container_pull",
)

container_pull(
    name = "distroless_cc",
    # Latest as of 10/21/2019. To update, remove this line, re-build, and copy the suggested digest.
    digest = "sha256:86f16733f25964c40dcd34edf14339ddbb2287af2f7c9dfad88f0366723c00d7",
    registry = "gcr.io",
    repository = "distroless/cc",
)

container_pull(
    name = "bionic",
    # Latest as of 10/21/2019. To update, remove this line, re-build, and copy the suggested digest.
    digest = "sha256:3e83eca7870ee14a03b8026660e71ba761e6919b6982fb920d10254688a363d4",
    registry = "index.docker.io",
    repository = "library/ubuntu",
    tag = "bionic",
)

# End of docker dependencies

load("//bazel:wasm.bzl", "wasm_dependencies")

wasm_dependencies()
