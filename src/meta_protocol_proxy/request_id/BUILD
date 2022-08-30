package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

load("@envoy//bazel:envoy_build_system.bzl", "envoy_cc_library")

envoy_cc_library(
    name = "request_id_interface",
    repository = "@envoy",
    hdrs = [
        "config.h",
        "request_id_extension.h",
    ],
    deps = [
        "@envoy//source/common/common:random_generator_lib",
        "@envoy//envoy/tracing:trace_reason_interface",
        "//src/meta_protocol_proxy/codec:codec_interface",
    ],
)
envoy_cc_library(
    name = "request_id_lib",
    repository = "@envoy",
    srcs = [
        "config.cc",
    ],
    deps = [
        ":request_id_interface",
        "//src/meta_protocol_proxy/codec:codec_interface",
    ],
)
