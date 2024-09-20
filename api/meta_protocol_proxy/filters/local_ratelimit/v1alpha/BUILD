# compile proto
load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

licenses(["notice"])  # Apache 2

api_proto_package(
     deps = [
        "//api/meta_protocol_proxy/config/route/v1alpha:pkg",
        "@com_github_cncf_xdsgithub_cncf_xds//udpa/annotations:pkg",
        "@envoy_api//envoy/type/v3:pkg",
        "@envoy_api//envoy/config/core/v3:pkg",
        "@envoy_api//envoy/config/route/v3:pkg",
     ],
)
