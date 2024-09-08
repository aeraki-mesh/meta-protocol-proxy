package(default_visibility = ["//visibility:public"])

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_binary",
)

ISTIO_EXTENSIONS = [
    "@io_istio_proxy//source/extensions/common/workload_discovery:api_lib",  # Experimental: WIP
    "@io_istio_proxy//source/extensions/filters/http/alpn:config_lib",
    "@io_istio_proxy//source/extensions/filters/http/istio_stats",
    "@io_istio_proxy//source/extensions/filters/http/peer_metadata:filter_lib",
    "@io_istio_proxy//source/extensions/filters/network/metadata_exchange:config_lib",
]

envoy_cc_binary(
    name = "envoy",
    repository = "@envoy",
    deps = ISTIO_EXTENSIONS + [
        "//src/meta_protocol_proxy:config",
        "//src/application_protocols/dubbo:config",
        "//src/application_protocols/thrift:config",
        "//src/application_protocols/brpc:config",
        "//src/application_protocols/trpc:config",
        "@envoy//source/exe:envoy_main_entry_lib",
    ],
)
