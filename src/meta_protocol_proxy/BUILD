package(default_visibility = ["//visibility:public"])

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)

envoy_cc_library(
    name = "config",
    repository = "@envoy",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":conn_manager_lib",
        ":codec_impl_lib",
        "//src/meta_protocol_proxy/codec:factory_lib",
        "//src/meta_protocol_proxy/route:route_config_provider_manager_interface",
        "//src/meta_protocol_proxy/route:rds_lib",
        "//src/meta_protocol_proxy/route:route_matcher",
        "//src/meta_protocol_proxy/route:route_config_lib",
        "//src/meta_protocol_proxy/filters:factory_base_lib",
        "//src/meta_protocol_proxy/filters/router:config",
        "//src/meta_protocol_proxy/filters/router:router_lib",
        "@envoy//envoy/registry",
        "@envoy//envoy/stats:stats_interface",
        "@envoy//envoy/stats:stats_macros",
        "@envoy//source/common/common:utility_lib",
        "@envoy//source/common/config:utility_lib",
        "@envoy//source/extensions/filters/network:well_known_names",
        "@envoy//source/extensions/filters/network/common:factory_base_lib",
    ],
)

envoy_cc_library(
    name = "conn_manager_lib",
    repository = "@envoy",
    srcs = [
        "active_message.cc",
        "conn_manager.cc",
    ],
    hdrs = [
        "active_message.h",
        "conn_manager.h",
    ],
    deps = [
        ":app_exception_lib",
        ":decoder_events_lib",
        ":decoder_lib",
        ":heartbeat_response_lib",
        ":stats_lib",
        "//api/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy/route:rds_interface",
        "//src/meta_protocol_proxy/route:route_interface",
        "//src/meta_protocol_proxy/filters:filter_interface",
        "@envoy//envoy/event:deferred_deletable",
        "@envoy//envoy/event:dispatcher_interface",
        "@envoy//envoy/network:connection_interface",
        "@envoy//envoy/network:filter_interface",
        "@envoy//envoy/stats:stats_interface",
        "@envoy//envoy/stats:timespan_interface",
        "@envoy//source/common/buffer:buffer_lib",
        "@envoy//source/common/buffer:watermark_buffer_lib",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:linked_object",
        "@envoy//source/common/common:logger_lib",
        "@envoy//source/common/network:filter_lib",
        "@envoy//source/common/stats:timespan_lib",
        "@envoy//source/common/stream_info:stream_info_lib",
    ],
)

envoy_cc_library(
    name = "decoder_events_lib",
    repository = "@envoy",
    hdrs = ["decoder_event_handler.h"],
    deps = [
        "@envoy//envoy/network:connection_interface",
        "@envoy//envoy/network:filter_interface",
        "@envoy//source/common/buffer:buffer_lib",
    ],
)

envoy_cc_library(
    name = "codec_impl_lib",
    repository = "@envoy",
    srcs = ["codec_impl.cc"],
    hdrs = ["codec_impl.h"],
    deps = [
        "//src/meta_protocol_proxy/codec:codec_interface",
        "@envoy//source/common/buffer:buffer_lib",
        "@envoy//source/common/http:header_map_lib",
    ],
)

envoy_cc_library(
    name = "decoder_lib",
    repository = "@envoy",
    srcs = ["decoder.cc"],
    hdrs = ["decoder.h"],
    deps = [
        ":decoder_events_lib",
        ":codec_impl_lib",
        "@envoy//source/common/buffer:buffer_lib",
        "@envoy//source/common/common:logger_lib",
    ],
)

envoy_cc_library(
    name = "stats_lib",
    repository = "@envoy",
    hdrs = ["stats.h"],
    deps = [
        "@envoy//envoy/stats:stats_interface",
        "@envoy//envoy/stats:stats_macros",
    ],
)

envoy_cc_library(
    name = "app_exception_lib",
    repository = "@envoy",
    srcs = ["app_exception.cc"],
    hdrs = ["app_exception.h"],
    deps = [
        "//src/meta_protocol_proxy/filters:filter_interface",
        "//src/meta_protocol_proxy/codec:codec_interface",
    ],
)

envoy_cc_library(
    name = "heartbeat_response_lib",
    repository = "@envoy",
    srcs = ["heartbeat_response.cc"],
    hdrs = ["heartbeat_response.h"],
    deps = [
        ":codec_impl_lib",
        "//src/meta_protocol_proxy/filters:filter_interface",
        "//src/meta_protocol_proxy/codec:codec_interface",
    ],
)


