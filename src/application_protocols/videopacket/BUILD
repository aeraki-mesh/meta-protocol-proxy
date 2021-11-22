load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)

# compile proto
load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

envoy_cc_library(
    name = "config",
    visibility = ["//:__pkg__"],
    repository = "@envoy",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":codec_lib",
        "@envoy//include/envoy/registry",
        "//src/meta_protocol_proxy/codec:factory_lib",
    ],
)

envoy_cc_library(
    name = "codec_lib",
    repository = "@envoy",
    srcs = ["videopacket_codec.cc"],
    hdrs = ["videopacket_codec.h"],
    deps = [
        "@envoy//include/envoy/buffer:buffer_interface",
        "@envoy//source/common/common:logger_lib",
        "@envoy//source/common/buffer:buffer_lib",
        "//src/meta_protocol_proxy/codec:codec_interface",
       "//jce:jce",
        ":static",
    ],
)

cc_import(
    name = "static",
    # hdrs = ["src/application_protocols/videopacket/video_packet.h"],
    hdrs = ["video_packet.h", "VideoCommHead.h"],
    # static_library = "src/application_protocols/videopacket/libvideo_packet.a",
    static_library = "libvideo_packet.a",
)
