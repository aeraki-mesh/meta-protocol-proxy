package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

load("@envoy//bazel:envoy_build_system.bzl", "envoy_cc_library")

envoy_cc_library(
    name = "config",
    repository = "@envoy",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":metadata_exchange",
        "//api/meta_protocol_proxy/filters/metadata_exchange/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy/filters:factory_base_lib",
        "//src/meta_protocol_proxy/filters:filter_config_interface",
        "@envoy//envoy/registry",
    ],
)

envoy_cc_library(
    name = "metadata_exchange",
    repository = "@envoy",
    srcs = ["metadata_exchange.cc"],
    hdrs = ["metadata_exchange.h"],
    deps = [
        "//api/meta_protocol_proxy/filters/metadata_exchange/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy/filters:filter_interface",
        "//src/meta_protocol_proxy/filters/common:base64_lib",
        "@envoy//source/common/common:logger_lib",
        "@envoy//envoy/stats:stats_interface",
	"@envoy//envoy/server:factory_context_interface",
	"@io_istio_proxy//extensions/common:proto_util",
    ],
)

exports_files(["base64.h"])
