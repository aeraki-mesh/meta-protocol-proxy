package(default_visibility = ["//visibility:public"])

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_cc_library(
    name = "tracer_interface",
    repository = "@envoy",
    hdrs = ["tracer.h"],
    deps = [
        "//src/meta_protocol_proxy/codec:codec_interface",
        "@envoy//envoy/tracing:trace_driver_interface",
        "@envoy//envoy/http:header_map_interface",
    ],
)

envoy_cc_library(
    name = "tracer_manager_interface",
    repository = "@envoy",
    hdrs = ["tracer_manager.h"],
    deps = [
        ":tracer_interface",
        "@envoy_api//envoy/config/trace/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "null_span_lib",
    repository = "@envoy",
    hdrs = [
        "null_span_impl.h",
    ],
    deps = [
        "@envoy//envoy/tracing:trace_driver_interface",
        "@envoy//source/common/common:empty_string",
    ],
)

envoy_cc_library(
    name = "common_values_lib",
    repository = "@envoy",
    hdrs = [
        "common_values.h",
    ],
    deps = [
        "@envoy//source/common/singleton:const_singleton",
    ],
)

envoy_cc_library(
    name = "tracer_lib",
    repository = "@envoy",
    srcs = [
        "tracer_impl.cc",
    ],
    hdrs = [
        "tracer_impl.h",
    ],
    deps = [
        ":common_values_lib",
        ":null_span_lib",
        ":tracer_interface",
        "//src/meta_protocol_proxy:codec_impl_lib",
        "@envoy//envoy/http:request_id_extension_interface",
        "@envoy//envoy/local_info:local_info_interface",
        "@envoy//envoy/runtime:runtime_interface",
        "@envoy//envoy/thread_local:thread_local_interface",
        "@envoy//envoy/upstream:cluster_manager_interface",
        "@envoy//source/common/buffer:zero_copy_input_stream_lib",
        "@envoy//source/common/common:base64_lib",
        "@envoy//source/common/common:macros",
        "@envoy//source/common/common:utility_lib",
        "@envoy//source/common/config:metadata_lib",
        "@envoy//source/common/formatter:substitution_formatter_lib",
        "@envoy//source/common/grpc:common_lib",
        "@envoy//source/common/http:codes_lib",
        "@envoy//source/common/http:header_map_lib",
        "@envoy//source/common/http:headers_lib",
        "@envoy//source/common/http:message_lib",
        "@envoy//source/common/http:utility_lib",
        "@envoy//source/common/json:json_loader_lib",
        "@envoy//source/common/protobuf:utility_lib",
        "@envoy//source/common/stream_info:utility_lib",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@envoy_api//envoy/type/metadata/v3:pkg_cc_proto",
        "@envoy_api//envoy/type/tracing/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "tracer_config_lib",
    repository = "@envoy",
    hdrs = [
        "tracer_config_impl.h",
    ],
    deps = [
        "@envoy//envoy/server:tracer_config_interface",
    ],
)

envoy_cc_library(
    name = "tracer_manager_lib",
    repository = "@envoy",
    srcs = [
        "tracer_manager_impl.cc",
    ],
    hdrs = [
        "tracer_manager_impl.h",
    ],
    deps = [
        ":tracer_lib",
        ":tracer_manager_interface",
        "@envoy//envoy/server:tracer_config_interface",
        "@envoy//envoy/singleton:instance_interface",
        "@envoy//source/common/common:minimal_logger_lib",
        "@envoy//source/common/config:utility_lib",
    ],
)
