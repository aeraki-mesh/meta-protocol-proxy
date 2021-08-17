package(default_visibility =  [
        "//src/meta_protocol_proxy:__pkg__",
        "//src/meta_protocol_proxy/filters:__pkg__",
        "//src/meta_protocol_proxy/filters/router:__pkg__",
    ],
)

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)

load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

envoy_cc_library(
    name = "route_interface",
    repository = "@envoy",
    hdrs = ["route.h"],
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
        ":route_interface",
        "//api/router/v1alpha:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "route_config_provider_manager_interface",
    repository = "@envoy",
    hdrs = ["route_config_provider_manager.h"],
    deps = [
        "//api/v1alpha:pkg_cc_proto",
        "//api/router/v1alpha:pkg_cc_proto",
        "@envoy//envoy/server:factory_context_interface",
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
        ":route_matcher_interface",
        ":route_matcher",
	":route_interface",
	"//api/v1alpha:pkg_cc_proto",
        "//api/router/v1alpha:pkg_cc_proto",
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
        "@envoy_api//envoy/admin/v3:pkg_cc_proto",
        "@envoy_api//envoy/api/v2:pkg_cc_proto",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/service/discovery/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "route_matcher_interface",
    repository = "@envoy",
    hdrs = ["route_matcher.h"],
    deps = [
        "//api/router/v1alpha:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "route_matcher",
    repository = "@envoy",
    srcs = ["route_matcher_impl.cc"],
    hdrs = ["route_matcher_impl.h"],
    deps = [
        ":route_matcher_interface",
        ":route_interface",
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



