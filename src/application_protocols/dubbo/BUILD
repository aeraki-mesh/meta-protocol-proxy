load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)

# compile proto
load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

api_proto_package(
    deps = ["@com_github_cncf_udpa//udpa/annotations:pkg"],
)

envoy_cc_library(
    name = "config",
    visibility = ["//:__pkg__"],
    repository = "@envoy",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":pkg_cc_proto",
        ":codec_lib",
        "@envoy//envoy/registry",
        "//src/meta_protocol_proxy/codec:factory_lib",
    ],
)

envoy_cc_library(
    name = "codec_lib",
    repository = "@envoy",
    srcs = ["dubbo_codec.cc"],
    hdrs = ["dubbo_codec.h"],
    deps = [
        "@envoy//envoy/buffer:buffer_interface",
        "@envoy//source/common/common:logger_lib",
        "@envoy//source/common/buffer:buffer_lib",
        "//src/meta_protocol_proxy/codec:codec_interface",
        ":protocol_interface",
        ":dubbo_protocol_impl_lib",
        ":dubbo_hessian2_serializer_impl_lib",
    ],
)

envoy_cc_library(
    name = "hessian_utils_lib",
    repository = "@envoy",
    srcs = ["hessian_utils.cc"],
    hdrs = ["hessian_utils.h"],
    external_deps = [
        "hessian2_codec_codec_impl",
        "hessian2_codec_object_codec_lib",
    ],
    deps = [
        "@envoy//envoy/buffer:buffer_interface",
        "@envoy//source/common/singleton:const_singleton",
    ],
)

envoy_cc_library(
    name = "message_lib",
    repository = "@envoy",
    srcs = [
        "message_impl.cc",
    ],
    hdrs = [
        "message.h",
        "message_impl.h",
    ],
    deps = [
        ":hessian_utils_lib",
        "@envoy//source/common/buffer:buffer_lib",
        "@envoy//source/common/http:header_map_lib",
    ],
)

envoy_cc_library(
    name = "metadata_lib",
    repository = "@envoy",
    hdrs = ["metadata.h"],
    external_deps = ["abseil_optional"],
    deps = [
        ":message_lib",
        "@envoy//source/common/buffer:buffer_lib",
        "@envoy//source/common/http:header_map_lib",
    ],
)

envoy_cc_library(
    name = "serializer_interface",
    repository = "@envoy",
    hdrs = [
        "protocol_constants.h",
        "serializer.h",
    ],
    deps = [
        ":hessian_utils_lib",
        ":message_lib",
        ":metadata_lib",
        "@envoy//envoy/buffer:buffer_interface",
        "@envoy//envoy/config:typed_config_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/config:utility_lib",
        "@envoy//source/common/singleton:const_singleton",
    ],
)

envoy_cc_library(
    name = "protocol_interface",
    repository = "@envoy",
    hdrs = ["protocol.h"],
    deps = [
        ":message_lib",
        ":metadata_lib",
        ":serializer_interface",
        "@envoy//envoy/config:typed_config_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/config:utility_lib",
        "@envoy//source/common/singleton:const_singleton",
    ],
)

envoy_cc_library(
    name = "dubbo_protocol_impl_lib",
    repository = "@envoy",
    srcs = ["dubbo_protocol_impl.cc"],
    hdrs = ["dubbo_protocol_impl.h"],
    deps = [
        ":protocol_interface",
        "@envoy//envoy/buffer:buffer_interface",
        "@envoy//source/common/singleton:const_singleton",
    ],
)

envoy_cc_library(
    name = "dubbo_hessian2_serializer_impl_lib",
    repository = "@envoy",
    srcs = ["dubbo_hessian2_serializer_impl.cc"],
    hdrs = [
        "dubbo_hessian2_serializer_impl.h",
    ],
    deps = [
        ":hessian_utils_lib",
        ":serializer_interface",
        "@envoy//source/common/singleton:const_singleton",
    ],
)
