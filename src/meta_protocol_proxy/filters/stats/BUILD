package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

load("@envoy//bazel:envoy_build_system.bzl", "envoy_cc_library")

envoy_cc_library(
    name = "config",
    repository = "@envoy",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":stats",
        "//api/meta_protocol_proxy/filters/stats/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy/filters:factory_base_lib",
        "//src/meta_protocol_proxy/filters:filter_config_interface",
        "@envoy//envoy/registry",
    ],
)

envoy_cc_library(
    name = "stats",
    repository = "@envoy",
    srcs = ["stats.cc"],
    hdrs = ["stats.h"],
    deps = [
        "//api/meta_protocol_proxy/filters/stats/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy/filters:filter_interface",
        "@envoy//source/common/common:logger_lib",
        "@envoy//envoy/stats:stats_interface",
	"@envoy//envoy/server:factory_context_interface",
    ],
)
