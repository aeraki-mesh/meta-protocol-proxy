package(default_visibility = ["//visibility:public"])

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)

envoy_cc_library(
    name = "codec_interface",
    repository = "@envoy",
    hdrs = ["codec.h"],
    deps = [
        "@envoy//envoy/buffer:buffer_interface",
    ],
)

envoy_cc_library(
    name = "factory_lib",
    repository = "@envoy",
    hdrs = ["factory.h"],
    deps = [
        ":codec_interface",
        "@envoy//envoy/config:typed_config_interface",
        "@envoy//source/common/common:macros",
        "@envoy//source/common/protobuf:utility_lib",
    ],
)
