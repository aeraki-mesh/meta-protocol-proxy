# compile proto
package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")
load("@envoy//bazel:envoy_build_system.bzl", "envoy_cc_library")

envoy_cc_library(
    name = "config",
    repository = "@envoy",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        "//api/meta_protocol_proxy/filters/local_ratelimit/v1alpha:pkg_cc_proto",
        "//api/meta_protocol_proxy/v1alpha:pkg_cc_proto",
        ":local_ratelimit",
        "//src/meta_protocol_proxy/filters:factory_base_lib",
        "//src/meta_protocol_proxy/filters:filter_config_interface",
        "@envoy//include/envoy/registry",
    ],
)

envoy_cc_library(
    name = "local_ratelimit",
    repository = "@envoy",
    srcs = ["local_ratelimit.cc"],
    hdrs = ["local_ratelimit.h"],
    deps = [
        "//api/meta_protocol_proxy/filters/local_ratelimit/v1alpha:pkg_cc_proto",
        "//api/meta_protocol_proxy/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy:app_exception_lib",
        "//src/meta_protocol_proxy/filters:filter_interface",
        "@envoy//source/common/common:logger_lib",
        "@envoy_api//envoy/extensions/filters/network/local_ratelimit/v3:pkg_cc_proto",

        "@envoy//include/envoy/network:filter_interface",

        # ":local_ratelimit_impl",
        ":local_ratelimit_manager",

        "@envoy//include/envoy/ratelimit:ratelimit_interface",
        "@envoy//include/envoy/stats:stats_macros",
        "@envoy//include/envoy/stats:stats_interface",
    ],
)

envoy_cc_library(
    name = "local_ratelimit_manager",
    repository = "@envoy",
    srcs = ["local_ratelimit_manager.cc"],
    hdrs = ["local_ratelimit_manager.h"],
    deps = [
        "//api/meta_protocol_proxy/filters/local_ratelimit/v1alpha:pkg_cc_proto",
        "//api/meta_protocol_proxy/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy:app_exception_lib",
        "//src/meta_protocol_proxy/filters:filter_interface",
        "@envoy//source/common/common:logger_lib",
        "@envoy_api//envoy/extensions/filters/network/local_ratelimit/v3:pkg_cc_proto",

        "@envoy//include/envoy/network:filter_interface",

        ":local_ratelimit_impl",

        "@envoy//include/envoy/ratelimit:ratelimit_interface",
        "@envoy//include/envoy/stats:stats_macros",
        "@envoy//include/envoy/stats:stats_interface",
    ],
)

envoy_cc_library(
    name = "local_ratelimit_impl",
    repository = "@envoy",
    srcs = ["local_ratelimit_impl.cc"],
    hdrs = ["local_ratelimit_impl.h"],
    deps = [
        "@envoy//source/common/common:thread_synchronizer_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy//include/envoy/event:dispatcher_interface",
        "@envoy//include/envoy/event:timer_interface",
        "@envoy_api//envoy/extensions/common/ratelimit/v3:pkg_cc_proto",
        "@envoy//include/envoy/ratelimit:ratelimit_interface",

        "@envoy//source/common/protobuf:protobuf",
        "//api/meta_protocol_proxy/filters/local_ratelimit/v1alpha:pkg_cc_proto",

    ],
)