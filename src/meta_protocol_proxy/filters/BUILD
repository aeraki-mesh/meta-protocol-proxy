package(default_visibility = ["//visibility:public"])

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)

envoy_cc_library(
    name = "filter_interface",
    repository = "@envoy",
    hdrs = ["filter.h"],
    deps = [
        "@envoy//envoy/buffer:buffer_interface",
        "@envoy//envoy/network:connection_interface",
        "@envoy//envoy/stream_info:stream_info_interface",
        "//src/meta_protocol_proxy:decoder_events_lib",
        "//src/meta_protocol_proxy/route:route_interface",
        "//src/meta_protocol_proxy/codec:codec_interface",
    ],
)

envoy_cc_library(
    name = "filter_config_interface",
    repository = "@envoy",
    hdrs = ["filter_config.h"],
    deps = [
        ":filter_interface",
        "@envoy//envoy/config:typed_config_interface",
        "@envoy//envoy/server:filter_config_interface",
        "@envoy//source/common/common:macros",
        "@envoy//source/common/protobuf:cc_wkt_protos",
    ],
)

envoy_cc_library(
    name = "factory_base_lib",
    repository = "@envoy",
    hdrs = ["factory_base.h"],
    deps = [
        ":filter_config_interface",
        "@envoy//source/common/protobuf:utility_lib",
    ],
)
