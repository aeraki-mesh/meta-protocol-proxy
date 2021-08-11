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
    name = "metadatamatchcriteria_lib",
    repository = "@envoy",
    srcs = ["metadatamatchcriteria_impl.cc"],
    hdrs = ["metadatamatchcriteria_impl.h"],
    deps = [
        "@envoy//envoy/router:router_interface",
    ],
)

envoy_cc_library(
    name = "tls_context_match_criteria_lib",
    repository = "@envoy",
    srcs = ["tls_context_match_criteria_impl.cc"],
    hdrs = ["tls_context_match_criteria_impl.h"],
    deps = [
        "@envoy//envoy/router:router_interface",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "config_lib",
    repository = "@envoy",
    srcs = ["config_impl.cc"],
    hdrs = ["config_impl.h","http_utility.h","filter_utility.h"],
    external_deps = ["abseil_optional"],
    deps = [
        ":config_utility_lib",
        ":header_formatter_lib",
        ":header_parser_lib",
        ":metadatamatchcriteria_lib",
        ":reset_header_parser_lib",
        ":retry_state_lib",
        ":router_ratelimit_lib",
        ":tls_context_match_criteria_lib",
        "@envoy//envoy/config:typed_metadata_interface",
        "@envoy//envoy/http:header_map_interface",
        "@envoy//envoy/router:router_interface",
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
    name = "config_utility_lib",
    repository = "@envoy",
    srcs = ["config_utility.cc"],
    hdrs = ["config_utility.h"],
    deps = [
        "@envoy//envoy/http:codes_interface",
        "@envoy//envoy/upstream:resource_manager_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:empty_string",
        "@envoy//source/common/common:matchers_lib",
        "@envoy//source/common/http:headers_lib",
        "@envoy//source/common/http:utility_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/type/matcher/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "context_lib",
    repository = "@envoy",
    srcs = ["context_impl.cc"],
    hdrs = ["context_impl.h"],
    deps = [
        "@envoy//envoy/router:context_interface",
        "@envoy//envoy/router:router_interface",
        "@envoy//envoy/stats:stats_macros",
    ],
)

envoy_cc_library(
    name = "debug_config_lib",
    repository = "@envoy",
    srcs = ["debug_config.cc"],
    hdrs = ["debug_config.h"],
    deps = [
        "@envoy//envoy/http:header_map_interface",
        "@envoy//envoy/stream_info:filter_state_interface",
        "@envoy//source/common/common:macros",
    ],
)

envoy_cc_library(
    name = "route_config_update_impl_lib",
    repository = "@envoy",
    srcs = ["route_config_update_receiver_impl.cc"],
    hdrs = ["route_config_update_receiver_impl.h"],
    deps = [
        ":config_lib",
        "@envoy//envoy/router:rds_interface",
        "@envoy//envoy/router:route_config_update_info_interface",
        "@envoy//envoy/server:factory_context_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:minimal_logger_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/service/discovery/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "vhds_lib",
    repository = "@envoy",
    srcs = ["vhds.cc"],
    hdrs = ["vhds.h"],
    deps = [
        ":config_lib",
        "@envoy//envoy/config:subscription_interface",
        "@envoy//envoy/http:codes_interface",
        "@envoy//envoy/local_info:local_info_interface",
        "@envoy//envoy/router:rds_interface",
        "@envoy//envoy/router:route_config_provider_manager_interface",
        "@envoy//envoy/router:route_config_update_info_interface",
        "@envoy//envoy/singleton:instance_interface",
        "@envoy//envoy/thread_local:thread_local_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:minimal_logger_lib",
        "@envoy//source/common/config:api_version_lib",
        "@envoy//source/common/config:subscription_base_interface",
        "@envoy//source/common/config:utility_lib",
        "@envoy//source/common/init:target_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy//source/common/router:route_config_update_impl_lib",
        "@envoy_api//envoy/api/v2/route:pkg_cc_proto",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
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
        ":config_lib",
        "@envoy//envoy/config:subscription_interface",
        "@envoy//envoy/http:codes_interface",
        "@envoy//envoy/local_info:local_info_interface",
        "@envoy//envoy/router:rds_interface",
        "@envoy//envoy/router:route_config_provider_manager_interface",
        "@envoy//envoy/router:route_config_update_info_interface",
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
    name = "scoped_config_lib",
    repository = "@envoy",
    srcs = ["scoped_config_impl.cc"],
    hdrs = ["scoped_config_impl.h"],
    external_deps = [
        "abseil_str_format",
    ],
    deps = [
        ":config_lib",
        "@envoy//envoy/router:rds_interface",
        "@envoy//envoy/router:scopes_interface",
        "@envoy//envoy/thread_local:thread_local_interface",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/filters/network/http_connection_manager/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "scoped_rds_lib",
    repository = "@envoy",
    srcs = ["scoped_rds.cc"],
    hdrs = ["scoped_rds.h"],
    deps = [
        ":rds_lib",
        ":scoped_config_lib",
        "@envoy//envoy/config:config_provider_interface",
        "@envoy//envoy/config:subscription_interface",
        "@envoy//envoy/router:route_config_provider_manager_interface",
        "@envoy//envoy/stats:stats_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:cleanup_lib",
        "@envoy//source/common/common:minimal_logger_lib",
        "@envoy//source/common/config:api_version_lib",
        "@envoy//source/common/config:config_provider_lib",
        "@envoy//source/common/config:resource_name_lib",
        "@envoy//source/common/config:subscription_base_interface",
        "@envoy//source/common/config:version_converter_lib",
        "@envoy//source/common/config:xds_resource_lib",
        "@envoy//source/common/init:manager_lib",
        "@envoy//source/common/init:watcher_lib",
        "@envoy_api//envoy/admin/v3:pkg_cc_proto",
        "@envoy_api//envoy/api/v2:pkg_cc_proto",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/filters/network/http_connection_manager/v3:pkg_cc_proto",
        "@envoy_api//envoy/service/discovery/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "retry_state_lib",
    repository = "@envoy",
    srcs = ["retry_state_impl.cc"],
    hdrs = ["retry_state_impl.h"],
    external_deps = ["abseil_optional"],
    deps = [
        "@envoy//envoy/event:timer_interface",
        "@envoy//envoy/http:codec_interface",
        "@envoy//envoy/http:header_map_interface",
        "@envoy//envoy/router:router_interface",
        "@envoy//envoy/runtime:runtime_interface",
        "@envoy//envoy/upstream:upstream_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:backoff_lib",
        "@envoy//source/common/common:utility_lib",
        "@envoy//source/common/grpc:common_lib",
        "@envoy//source/common/http:codes_lib",
        "@envoy//source/common/http:header_utility_lib",
        "@envoy//source/common/http:headers_lib",
        "@envoy//source/common/http:utility_lib",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "router_lib",
    repository = "@envoy",
    srcs = [
        "router.cc",
        "upstream_request.cc",
    ],
    hdrs = [
        "router.h",
        "upstream_request.h",
    ],
    deps = [
        ":config_lib",
        ":context_lib",
        ":debug_config_lib",
        ":header_parser_lib",
        ":retry_state_lib",
        "@envoy//envoy/event:dispatcher_interface",
        "@envoy//envoy/event:timer_interface",
        "@envoy//envoy/grpc:status",
        "@envoy//envoy/http:codec_interface",
        "@envoy//envoy/http:codes_interface",
        "@envoy//envoy/http:conn_pool_interface",
        "@envoy//envoy/http:filter_interface",
        "@envoy//envoy/local_info:local_info_interface",
        "@envoy//envoy/router:shadow_writer_interface",
        "@envoy//envoy/runtime:runtime_interface",
        "@envoy//envoy/server:filter_config_interface",
        "@envoy//envoy/stats:stats_interface",
        "@envoy//envoy/stats:stats_macros",
        "@envoy//envoy/upstream:cluster_manager_interface",
        "@envoy//envoy/upstream:upstream_interface",
        "@envoy//source/common/access_log:access_log_lib",
        "@envoy//source/common/buffer:watermark_buffer_lib",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:cleanup_lib",
        "@envoy//source/common/common:empty_string",
        "@envoy//source/common/common:enum_to_int",
        "@envoy//source/common/common:hash_lib",
        "@envoy//source/common/common:hex_lib",
        "@envoy//source/common/common:linked_object",
        "@envoy//source/common/common:minimal_logger_lib",
        "@envoy//source/common/common:scope_tracker",
        "@envoy//source/common/common:utility_lib",
        "@envoy//source/common/grpc:common_lib",
        "@envoy//source/common/http:codes_lib",
        "@envoy//source/common/http:header_map_lib",
        "@envoy//source/common/http:headers_lib",
        "@envoy//source/common/http:message_lib",
        "@envoy//source/common/http:utility_lib",
        "@envoy//source/common/network:application_protocol_lib",
        "@envoy//source/common/network:socket_option_factory_lib",
        "@envoy//source/common/network:transport_socket_options_lib",
        "@envoy//source/common/network:upstream_socket_options_filter_state_lib",
        "@envoy//source/common/stream_info:stream_info_lib",
        "@envoy//source/common/stream_info:uint32_accessor_lib",
        "@envoy//source/common/tracing:http_tracer_lib",
        "@envoy//source/common/upstream:load_balancer_lib",
        "@envoy//source/extensions/common/proxy_protocol:proxy_protocol_header_lib",
        "@envoy_api//envoy/extensions/filters/http/router/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "router_ratelimit_lib",
    repository = "@envoy",
    srcs = ["router_ratelimit.cc"],
    hdrs = ["router_ratelimit.h"],
    deps = [
        ":config_utility_lib",
        "@envoy//envoy/ratelimit:ratelimit_interface",
        "@envoy//envoy/router:router_interface",
        "@envoy//envoy/router:router_ratelimit_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:empty_string",
        "@envoy//source/common/config:metadata_lib",
        "@envoy//source/common/config:utility_lib",
        "@envoy//source/common/http:header_utility_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "shadow_writer_lib",
    repository = "@envoy",
    srcs = ["shadow_writer_impl.cc"],
    hdrs = ["shadow_writer_impl.h"],
    deps = [
        "@envoy//envoy/router:shadow_writer_interface",
        "@envoy//envoy/upstream:cluster_manager_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/http:headers_lib",
    ],
)

envoy_cc_library(
    name = "header_parser_lib",
    repository = "@envoy",
    srcs = ["header_parser.cc"],
    hdrs = ["header_parser.h"],
    deps = [
        ":header_formatter_lib",
        "@envoy//envoy/http:header_map_interface",
        "@envoy//source/common/http:header_utility_lib",
        "@envoy//source/common/http:headers_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "header_formatter_lib",
    repository = "@envoy",
    srcs = ["header_formatter.cc"],
    hdrs = ["header_formatter.h"],
    external_deps = ["abseil_optional"],
    deps = [
        "@envoy//envoy/router:string_accessor_interface",
        "@envoy//envoy/stream_info:filter_state_interface",
        "@envoy//envoy/stream_info:stream_info_interface",
        "@envoy//source/common/common:minimal_logger_lib",
        "@envoy//source/common/common:utility_lib",
        "@envoy//source/common/config:metadata_lib",
        "@envoy//source/common/formatter:substitution_formatter_lib",
        "@envoy//source/common/http:header_map_lib",
        "@envoy//source/common/json:json_loader_lib",
    ],
)

envoy_cc_library(
    name = "reset_header_parser_lib",
    repository = "@envoy",
    srcs = ["reset_header_parser.cc"],
    hdrs = ["reset_header_parser.h"],
    deps = [
        "@envoy//envoy/common:time_interface",
        "@envoy//envoy/http:header_map_interface",
        "@envoy//envoy/router:router_interface",
        "@envoy//source/common/http:headers_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "string_accessor_lib",
    repository = "@envoy",
    hdrs = ["string_accessor_impl.h"],
    deps = [
        "@envoy//envoy/router:string_accessor_interface",
    ],
)

envoy_cc_library(
    name = "delegating_route_lib",
    repository = "@envoy",
    srcs = ["delegating_route_impl.cc"],
    hdrs = ["delegating_route_impl.h"],
    deps = [
        "@envoy//envoy/router:router_interface",
    ],
)

