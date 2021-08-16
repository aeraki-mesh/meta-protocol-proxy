package(default_visibility =  [
        "//src/meta_protocol_proxy:__pkg__",
        "//src/meta_protocol_proxy/filters:__pkg__",
    ],
)

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)

load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

envoy_cc_library(
    name = "router_interface",
    repository = "@envoy",
    hdrs = ["router.h"],
    deps = [
        "//src/meta_protocol_proxy/codec:codec_interface",
        "@envoy//envoy/router:router_interface",
    ],
)

envoy_cc_library(
    name = "rds_interface",
    repository = "@envoy",
    hdrs = ["rds.h"],
    deps = [
        ":router_interface",
        "//api/router/v1alpha:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "route_config_provider_manager_interface",
    repository = "@envoy",
    hdrs = ["route_config_provider_manager.h"],
    deps = [
        ":rds_interface",
        "//api/v1alpha:pkg_cc_proto",
        "//api/router/v1alpha:pkg_cc_proto",
        "@envoy//envoy/event:dispatcher_interface",
        "@envoy//envoy/json:json_object_interface",
        "@envoy//envoy/local_info:local_info_interface",
        "@envoy//envoy/runtime:runtime_interface",
        "@envoy//envoy/server:filter_config_interface",
        "@envoy//envoy/stats:stats_interface",
        "@envoy//envoy/thread_local:thread_local_interface",
        "@envoy//envoy/upstream:cluster_manager_interface",
    ],
)

envoy_cc_library(
    name = "route_config_update_info_interface",
    repository = "@envoy",
    hdrs = ["route_config_update_receiver.h"],
    external_deps = ["abseil_optional"],
    deps = [
        ":rds_interface",
        "//api/router/v1alpha:pkg_cc_proto",
        "@envoy//envoy/common:time_interface",
        "@envoy//source/common/protobuf",
        "@envoy_api//envoy/service/discovery/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "route_config_lib",
    repository = "@envoy",
    srcs = ["config_impl.cc"],
    hdrs = ["config_impl.h"],
    external_deps = ["abseil_optional"],
    deps = [
	    "//api/v1alpha:pkg_cc_proto",
	    "//src/meta_protocol_proxy/filters/router:router_interface",
        "@envoy//envoy/config:typed_metadata_interface",
        "@envoy//envoy/http:header_map_interface",
        "@envoy//envoy/runtime:runtime_interface",
        "@envoy//envoy/server:filter_config_interface",  # TODO(rodaine): break dependency on server
        "@envoy//envoy/upstream:cluster_manager_interface",
        "@envoy//envoy/upstream:upstream_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:empty_string",
        "@envoy//source/common/common:hash_lib",
        "@envoy//source/common/common:matchers_lib",
        "@envoy//source/common/common:utility_lib",
        "@envoy//source/common/config:metadata_lib",
        "@envoy//source/common/config:utility_lib",
        "@envoy//source/common/config:well_known_names",
        "@envoy//source/common/http:hash_policy_lib",
        "@envoy//source/common/http:header_utility_lib",
        "@envoy//source/common/http:headers_lib",
        "@envoy//source/common/http:path_utility_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy//source/common/tracing:http_tracer_lib",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/type/matcher/v3:pkg_cc_proto",
        "@envoy_api//envoy/type/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "route_config_update_impl_lib",
    repository = "@envoy",
    srcs = ["route_config_update_receiver_impl.cc"],
    hdrs = ["route_config_update_receiver_impl.h"],
    deps = [
        ":route_config_lib",
        ":rds_interface",
        ":route_config_update_info_interface",
        "@envoy//envoy/server:factory_context_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:minimal_logger_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/service/discovery/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "rds_lib",
    repository = "@envoy",
    srcs = ["rds_impl.cc"],
    hdrs = ["rds_impl.h"],
    deps = [
        ":route_config_lib",
        ":route_config_update_impl_lib",
        ":rds_interface",
        ":route_config_provider_manager_interface",
        ":route_config_update_info_interface",
        "@envoy//envoy/config:subscription_interface",
        "@envoy//envoy/http:codes_interface",
        "@envoy//envoy/local_info:local_info_interface",
        "@envoy//envoy/server:admin_interface",
        "@envoy//envoy/singleton:instance_interface",
        "@envoy//envoy/thread_local:thread_local_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:callback_impl_lib",
        "@envoy//source/common/common:cleanup_lib",
        "@envoy//source/common/common:minimal_logger_lib",
        "@envoy//source/common/config:api_version_lib",
        "@envoy//source/common/config:subscription_base_interface",
        "@envoy//source/common/config:subscription_factory_lib",
        "@envoy//source/common/config:utility_lib",
        "@envoy//source/common/config:version_converter_lib",
        "@envoy//source/common/init:manager_lib",
        "@envoy//source/common/init:target_lib",
        "@envoy//source/common/init:watcher_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy//source/common/router:route_config_update_impl_lib",
        "@envoy//source/common/router:vhds_lib",
        "@envoy_api//envoy/admin/v3:pkg_cc_proto",
        "@envoy_api//envoy/api/v2:pkg_cc_proto",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/filters/network/http_connection_manager/v3:pkg_cc_proto",
        "@envoy_api//envoy/service/discovery/v3:pkg_cc_proto",
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



