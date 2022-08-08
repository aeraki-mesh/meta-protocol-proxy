load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)

# compile proto
load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

api_proto_package(
    deps = ["@com_github_cncf_udpa//udpa/annotations:pkg"],
)

envoy_cc_library(
    name = "config",
    visibility = ["//visibility:public"],
    repository = "@envoy",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":pkg_cc_proto",
        ":codec_lib",
        "@envoy//envoy/registry",
        "//src/meta_protocol_proxy/codec:factory_lib",
    ],
)

envoy_cc_library(
    name = "codec_lib",
    repository = "@envoy",
    srcs = ["zork_codec.cc"],
    hdrs = ["zork_codec.h"],
    deps = [
        "@envoy//envoy/buffer:buffer_interface",
        "@envoy//source/common/common:logger_lib",
        "@envoy//source/common/buffer:buffer_lib",
        "//src/meta_protocol_proxy/codec:codec_interface",
        ":protocol",
    ],
)

envoy_cc_library(
    name = "protocol",
    repository = "@envoy",
    srcs = ["protocol.cc"],
    hdrs = ["protocol.h"],
    deps = [
        "@envoy//source/common/buffer:buffer_lib",
        "@envoy//source/common/common:minimal_logger_lib",
    ]
)
