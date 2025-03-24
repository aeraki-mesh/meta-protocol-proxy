load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

licenses(["notice"])  # Apache 2

api_proto_package(
    deps = [
        "@envoy_api//envoy/config/bootstrap/v3:pkg",
        "@envoy_api//envoy/config/core/v3:pkg",
        "@envoy_api//envoy/type/v3:pkg",
        "@envoy_api//envoy/admin/v3:pkg",
        "@com_github_cncf_xds//udpa/annotations:pkg",
    ],
)

