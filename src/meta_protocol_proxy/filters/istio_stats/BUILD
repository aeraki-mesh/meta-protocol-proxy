package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

load("@envoy//bazel:envoy_build_system.bzl", "envoy_cc_library")

envoy_cc_library(
    name = "config",
    repository = "@envoy",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":stats_filter_lib",
        "//api/meta_protocol_proxy/filters/istio_stats/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy/filters:factory_base_lib",
        "//src/meta_protocol_proxy/filters:filter_config_interface",
        "@envoy//envoy/registry",
    ],
)

envoy_cc_library(
    name = "stats_filter_lib",
    repository = "@envoy",
    srcs = ["stats_filter.cc"],
    hdrs = ["stats_filter.h"],
    deps = [
        ":istio_stats_lib",
        "//api/meta_protocol_proxy/filters/istio_stats/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy/filters:filter_interface",
        "//src/meta_protocol_proxy/filters/common:base64_lib",
        "@envoy//source/common/common:logger_lib",
        "@envoy//envoy/stats:stats_interface",
        "@envoy//envoy/server:factory_context_interface",
        "@io_istio_proxy//extensions/common:proto_util",
        "@io_istio_proxy//extensions/common:context",
    ],
)

envoy_cc_library(
    name = "istio_stats_lib",
    repository = "@envoy",
    srcs = ["istio_stats.cc"],
    hdrs = ["istio_stats.h"],
    deps = [
        "//src/meta_protocol_proxy/codec:codec_interface",		    
        "@envoy//envoy/stats:stats_interface",
        "@envoy//source/common/common:linked_object",
        "@envoy//source/common/stats:symbol_table_lib",
        "@envoy//source/common/stats:utility_lib",
        "@envoy//envoy/server:factory_context_interface",
        "@io_istio_proxy//extensions/common:proto_util",
        "@io_istio_proxy//extensions/common:context",
    ],
)
