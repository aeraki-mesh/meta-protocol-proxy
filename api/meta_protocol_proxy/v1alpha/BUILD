load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

# compile proto
load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

licenses(["notice"])  # Apache 2

api_proto_package(
    deps = [
        "//api/meta_protocol_proxy/config/route/v1alpha:pkg",
        "@envoy_api//envoy/config/accesslog/v3:pkg",
        "@envoy_api//envoy/config/core/v3:pkg",
        "@envoy_api//envoy/config/trace/v3:pkg",
        "@envoy_api//envoy/config/route/v3:pkg",
        "@envoy_api//envoy/type/matcher/v3:pkg",
        "@envoy_api//envoy/type/v3:pkg",
        "@com_github_cncf_udpa//udpa/annotations:pkg",
    ],
)

envoy_cc_library(
    name = "v1alpha",
    repository = "@envoy",
    deps = [
        ":pkg_cc_proto",
        "//api/meta_protocol_proxy/config/route/v1alpha:pkg_cc_proto",
    ],
)

