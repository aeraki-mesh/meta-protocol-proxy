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
	"@io_istio_proxy//extensions/access_log_policy:access_log_policy_lib",
        "@io_istio_proxy//extensions/attributegen:attributegen_plugin",
        "@io_istio_proxy//extensions/metadata_exchange:metadata_exchange_lib",
        "@io_istio_proxy//extensions/stackdriver:stackdriver_plugin",
        "@io_istio_proxy//extensions/stats:stats_plugin",
        "@io_istio_proxy//source/extensions/filters/http/alpn:config_lib",
        "@io_istio_proxy//source/extensions/filters/http/authn:filter_lib",
        "@io_istio_proxy//source/extensions/filters/http/istio_stats",
        "@io_istio_proxy//source/extensions/filters/network/forward_downstream_sni:config_lib",
        "@io_istio_proxy//source/extensions/filters/network/metadata_exchange:config_lib",
        "@io_istio_proxy//source/extensions/filters/network/sni_verifier:config_lib",
        "@io_istio_proxy//source/extensions/filters/network/tcp_cluster_rewrite:config_lib",
        "@io_istio_proxy//src/envoy/http/baggage_handler:config_lib",  # Experimental: Ambient
        "@io_istio_proxy//src/envoy/metadata_to_peer_node:config_lib",  # Experimental: Ambient
        "@io_istio_proxy//src/envoy/set_internal_dst_address:filter_lib",  # Experimental: Ambient
        "@io_istio_proxy//src/envoy/tls_passthrough:filter_lib",  # Experimental: Ambient
        "@io_istio_proxy//src/envoy/workload_metadata:config_lib",  # Experimental: Ambient
        "@envoy//source/exe:envoy_main_entry_lib",
    ],
)
