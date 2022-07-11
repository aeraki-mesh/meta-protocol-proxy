package(default_visibility =  [
        "//src/meta_protocol_proxy:__pkg__",
    ],
)

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)

load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

envoy_cc_library(
    name = "config",
    repository = "@envoy",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":router_lib",
        "//api/meta_protocol_proxy/v1alpha:pkg_cc_proto",
        "//api/meta_protocol_proxy/filters/router/v1alpha:pkg_cc_proto",
        "//src/meta_protocol_proxy/filters:factory_base_lib",
        "//src/meta_protocol_proxy/filters:filter_config_interface",
        "@envoy//envoy/registry",
    ],
)

envoy_cc_library(
    name = "router_interface",
    repository = "@envoy",
    hdrs = ["router.h"],
    deps = [
        "//src/meta_protocol_proxy/filters:filter_interface",
        "@envoy//envoy/tcp:conn_pool_interface",
        "@envoy//envoy/upstream:thread_local_cluster_interface",
    ],
)

envoy_cc_library(
    name = "upstream_request_lib",
    repository = "@envoy",
    srcs = ["upstream_request.cc"],
    hdrs = ["upstream_request.h"],
    deps = [
        ":router_interface",
        "//src/meta_protocol_proxy:app_exception_lib",
        "//src/meta_protocol_proxy/filters:filter_interface",
        "//src/meta_protocol_proxy/route:route_interface",
        "@envoy//envoy/tcp:conn_pool_interface",
        "@envoy//envoy/upstream:cluster_manager_interface",
        "@envoy//envoy/upstream:thread_local_cluster_interface",
        "@envoy//source/common/common:logger_lib",
    ],
)

envoy_cc_library(
    name = "router_lib",
    repository = "@envoy",
    srcs = ["router_impl.cc"],
    hdrs = ["router_impl.h"],
    deps = [
        ":router_interface",
        ":upstream_request_lib",
        "//src/meta_protocol_proxy:app_exception_lib",
        "//src/meta_protocol_proxy/filters:filter_interface",
        "//src/meta_protocol_proxy/route:route_interface",
        "@envoy//envoy/tcp:conn_pool_interface",
        "@envoy//envoy/upstream:cluster_manager_interface",
        "@envoy//envoy/upstream:load_balancer_interface",
        "@envoy//envoy/upstream:thread_local_cluster_interface",
        "@envoy//source/common/common:logger_lib",
        "@envoy//source/common/http:header_utility_lib",
        "@envoy//source/common/router:metadatamatchcriteria_lib",
        "@envoy//source/common/upstream:load_balancer_lib",
    ],
)



