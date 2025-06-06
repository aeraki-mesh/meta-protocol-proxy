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
    strip_prefix = "proxy-1.21.0",
    sha256 = "007aa638869bad4e9b4a98c56518d033f59a67774e098307dffaf562832d3fa6",
    url = "https://github.com/istio/proxy/archive/refs/tags/1.21.0.tar.gz",
)

# 1. Determine SHA256 `wget https://github.com/envoyproxy/envoy/archive/$COMMIT.tar.gz && sha256sum $COMMIT.tar.gz`
# 2. Update .bazelversion, envoy.bazelrc and .bazelrc if needed.
#
# Commit date: 2024-08-22
ENVOY_SHA = "2bb4cf1997597a981bc2bf3e500a9ae35787911e"

ENVOY_SHA256 = "0b2dc5ae343dd70f13730c16f9025ce4787f4ec60a8a4a1a5bdbab7f378506fc"

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

envoy_dependencies_extra(ignore_root_user_error = True)

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