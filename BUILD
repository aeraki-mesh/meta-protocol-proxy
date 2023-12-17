package(default_visibility = ["//visibility:public"])

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_binary",
)

envoy_cc_binary(
    name = "envoy",
    repository = "@envoy",
    deps = [
        "//src/meta_protocol_proxy:config",
        "//src/application_protocols/dubbo:config",
        "//src/application_protocols/thrift:config",
        "//src/application_protocols/brpc:config",
        "//src/application_protocols/trpc:config",
        "@io_istio_proxy//extensions/access_log_policy:access_log_policy_lib",
        "@io_istio_proxy//extensions/metadata_exchange:metadata_exchange_lib",
        "@io_istio_proxy//extensions/stackdriver:stackdriver_plugin",
        "@io_istio_proxy//source/extensions/common/workload_discovery:api_lib",  # Experimental: WIP
        "@io_istio_proxy//source/extensions/filters/http/alpn:config_lib",
        "@io_istio_proxy//source/extensions/filters/http/authn:filter_lib",
        "@io_istio_proxy//source/extensions/filters/http/connect_authority",  # Experimental: ambient
        "@io_istio_proxy//source/extensions/filters/http/connect_baggage",  # Experimental: ambient
        "@io_istio_proxy//source/extensions/filters/http/istio_stats",
        "@io_istio_proxy//source/extensions/filters/listener/set_internal_dst_address:filter_lib",  # Experimental: ambient
        "@io_istio_proxy//source/extensions/filters/network/forward_downstream_sni:config_lib",
        "@io_istio_proxy//source/extensions/filters/network/istio_authn:config_lib",
        "@io_istio_proxy//source/extensions/filters/network/metadata_exchange:config_lib",
        "@io_istio_proxy//source/extensions/filters/network/sni_verifier:config_lib",
        "@io_istio_proxy//source/extensions/filters/network/tcp_cluster_rewrite:config_lib",
        "@envoy//source/exe:envoy_main_entry_lib",
    ],
)
