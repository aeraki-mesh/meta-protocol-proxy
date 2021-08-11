package(default_visibility =  [
        "//src/meta_protocol_proxy/filters/router/rds:__pkg__",
    ],
)

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)

envoy_cc_library(
    name = "context_interface",
    repository = "@envoy",
    hdrs = ["context.h"],
)

envoy_cc_library(
    name = "rds_interface",
    repository = "@envoy",
    hdrs = ["rds.h"],
    deps = [
        ":router_interface",
        "@envoy//envoy/http:filter_interface",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "route_config_provider_manager_interface",
    repository = "@envoy",
    hdrs = ["route_config_provider_manager.h"],
    deps = [
        ":rds_interface",
        "@envoy//envoy/event:dispatcher_interface",
        "@envoy//envoy/json:json_object_interface",
        "@envoy//envoy/local_info:local_info_interface",
        "@envoy//envoy/runtime:runtime_interface",
        "@envoy//envoy/server:filter_config_interface",
        "@envoy//envoy/stats:stats_interface",
        "@envoy//envoy/thread_local:thread_local_interface",
        "@envoy//envoy/upstream:cluster_manager_interface",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/filters/network/http_connection_manager/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "route_config_update_info_interface",
    repository = "@envoy",
    hdrs = ["route_config_update_receiver.h"],
    external_deps = ["abseil_optional"],
    deps = [
        ":rds_interface",
        "@envoy//envoy/common:time_interface",
        "@envoy//source/common/protobuf",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/service/discovery/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "router_interface",
    repository = "@envoy",
    hdrs = ["router.h"],
    external_deps = ["abseil_optional"],
    deps = [
        ":internal_redirect_interface",
        "@envoy//envoy/access_log:access_log_interface",
        "@envoy//envoy/common:conn_pool_interface",
        "@envoy//envoy/common:matchers_interface",
        "@envoy//envoy/common:time_interface",
        "@envoy//envoy/config:typed_metadata_interface",
        "@envoy//envoy/http:codec_interface",
        "@envoy//envoy/http:codes_interface",
        "@envoy//envoy/http:conn_pool_interface",
        "@envoy//envoy/http:hash_policy_interface",
        "@envoy//envoy/http:header_map_interface",
        "@envoy//envoy/tcp:conn_pool_interface",
        "@envoy//envoy/tracing:http_tracer_interface",
        "@envoy//envoy/upstream:resource_manager_interface",
        "@envoy//envoy/upstream:retry_interface",
        "@envoy//source/common/protobuf",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/type/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "scopes_interface",
    repository = "@envoy",
    hdrs = ["scopes.h"],
    deps = [
        ":router_interface",
        "@envoy//envoy/config:config_provider_interface",
        "@envoy//envoy/http:header_map_interface",
    ],
)

envoy_cc_library(
    name = "router_ratelimit_interface",
    repository = "@envoy",
    hdrs = ["router_ratelimit.h"],
    deps = [
        "@envoy//envoy/http:filter_interface",
        "@envoy//envoy/ratelimit:ratelimit_interface",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "shadow_writer_interface",
    repository = "@envoy",
    hdrs = ["shadow_writer.h"],
    deps = [
        "@envoy//envoy/http:async_client_interface",
        "@envoy//envoy/http:message_interface",
    ],
)

envoy_cc_library(
    name = "string_accessor_interface",
    repository = "@envoy",
    hdrs = ["string_accessor.h"],
    external_deps = ["abseil_optional"],
    deps = [
        "@envoy//envoy/stream_info:filter_state_interface",
    ],
)

envoy_cc_library(
    name = "internal_redirect_interface",
    repository = "@envoy",
    hdrs = ["internal_redirect.h"],
    deps = [
        "@envoy//envoy/config:typed_config_interface",
        "@envoy//envoy/stream_info:filter_state_interface",
        "@envoy//source/common/common:minimal_logger_lib",
    ],
)
