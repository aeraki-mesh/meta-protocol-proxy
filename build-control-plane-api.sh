#!/bin/bash
chmod +x bazel/get_workspace_status
export CC=clang-10
export CXX=clang++-10

export USE_BAZEL_VERSION=$(cat .bazelversion)

bazel build //api/meta_protocol_proxy/v1alpha:pkg_go_proto
bazel build //api/meta_protocol_proxy/filters/router/v1alpha:pkg_go_proto
bazel build //api/meta_protocol_proxy/config/route/v1alpha:pkg_go_proto
