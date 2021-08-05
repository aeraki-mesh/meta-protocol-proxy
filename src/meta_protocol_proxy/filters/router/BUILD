licenses(["notice"])  # Apache 2

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_binary",
    "envoy_cc_library",
    "envoy_cc_test",
)
load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

package(default_visibility = ["//visibility:public"])

envoy_cc_library(
    name = "router_interface",
    repository = "@envoy",
    hdrs = ["router.h"],
    deps = [
        "@envoy//envoy/router:router_interface",
    ],
)

envoy_cc_library(
    name = "route_matcher_interface",
    repository = "@envoy",
    hdrs = ["route.h"],
    deps = [
        ":router_interface",
        "@envoy//envoy/config:typed_config_interface",
        "@envoy//envoy/server:filter_config_interface",
        "@envoy//source/common/config:utility_lib",
        "@envoy//source/common/singleton:const_singleton",
        "//api/v1alpha:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "route_matcher",
    repository = "@envoy",
    srcs = ["route_matcher.cc"],
    hdrs = ["route_matcher.h"],
    deps = [
        ":route_matcher_interface",
        ":router_interface",
        "@envoy//envoy/router:router_interface",
        "@envoy//source/common/common:logger_lib",
        "@envoy//source/common/common:matchers_lib",
        "@envoy//source/common/http:header_utility_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/type/v3:pkg_cc_proto",
        "//src/meta_protocol_proxy/codec:codec_interface",
	    "//src/meta_protocol_proxy:codec_impl_lib",
	    "//api/v1alpha:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "config",
    repository = "@envoy",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":router_lib",
        "@envoy//envoy/registry",
        "//src/meta_protocol_proxy/filters:factory_base_lib",
        "//src/meta_protocol_proxy/filters:filter_config_interface",
        "//api/v1alpha:pkg_cc_proto",
        "//api/router/v1alpha:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "router_lib",
    repository = "@envoy",
    srcs = ["router_impl.cc"],
    hdrs = ["router_impl.h"],
    deps = [
        ":router_interface",
        "@envoy//envoy/tcp:conn_pool_interface",
        "@envoy//envoy/upstream:cluster_manager_interface",
        "@envoy//envoy/upstream:load_balancer_interface",
        "@envoy//envoy/upstream:thread_local_cluster_interface",
        "@envoy//source/common/common:logger_lib",
        "@envoy//source/common/http:header_utility_lib",
        "@envoy//source/common/router:metadatamatchcriteria_lib",
        "@envoy//source/common/upstream:load_balancer_lib",
        "//src/meta_protocol_proxy:app_exception_lib",
        "//src/meta_protocol_proxy/filters:filter_interface",
    ],
)



