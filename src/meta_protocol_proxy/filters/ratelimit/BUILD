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
        "//api/meta_protocol_proxy/filters/ratelimit/v1alpha:pkg_cc_proto",
        "//api/meta_protocol_proxy/v1alpha:pkg_cc_proto",
        ":ratelimit",
        "//src/meta_protocol_proxy/filters:factory_base_lib",
        "//src/meta_protocol_proxy/filters:filter_config_interface",
        "@envoy//include/envoy/registry",
    ],
)

envoy_cc_library(
    name = "ratelimit",
    repository = "@envoy",
    srcs = ["ratelimit.cc"],
    hdrs = ["ratelimit.h"],
    deps = [
        "//api/meta_protocol_proxy/filters/ratelimit/v1alpha:pkg_cc_proto",
        "//api/meta_protocol_proxy/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy:app_exception_lib",
        "//src/meta_protocol_proxy/filters:filter_interface",
        "@envoy//include/envoy/upstream:cluster_manager_interface",
        "@envoy//source/common/common:logger_lib",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_protobuf//:protobuf",
        "@envoy_api//envoy/service/ratelimit/v3:pkg_cc_grpc",

        "@envoy//include/envoy/upstream:load_balancer_interface",
        "@envoy//include/envoy/upstream:thread_local_cluster_interface",
        "@envoy//source/common/upstream:load_balancer_lib",
        "@envoy//include/envoy/stats:stats_interface",
        "@envoy//include/envoy/network:connection_interface",
    ],
)

# envoy_cc_library(
#     name = "client",
#     repository = "@envoy",
#     srcs = ["client.cc"],
#     hdrs = ["client.h"],
#     deps = [
#         "@envoy//source/common/common:logger_lib",
#         "@envoy//include/envoy/grpc:async_client_interface",
#         "@envoy//include/envoy/tracing:http_tracer_interface",
#     ],
# )
