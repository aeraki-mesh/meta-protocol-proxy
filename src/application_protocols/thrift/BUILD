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
    repository = "@envoy",
    visibility = ["//:__pkg__"],
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
    srcs = ["thrift_codec.cc"],
    hdrs = [
    "thrift_codec.h",
    "conn_state.h",
    "decoder_events.h",
    "metadata.h",
    "protocol.h",
    "thrift.h",
    "thrift_object.h",
    "tracing.h",
    "transport.h",
    ],
    deps = [
        "@envoy//envoy/registry",
	"@envoy//source/common/config:utility_lib",
        "@envoy//envoy/buffer:buffer_interface",
        "@envoy//source/common/common:logger_lib",
        "@envoy//source/common/buffer:buffer_lib",
        "//src/meta_protocol_proxy/codec:codec_interface",
    ],
)
