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
        #"@io_istio_proxy//src/envoy/extensions/wasm:wasm_lib",
        "@envoy//source/extensions/common/wasm:wasm_lib",
        "@io_istio_proxy//src/envoy/http/alpn:config_lib",
        "@io_istio_proxy//src/envoy/http/authn:filter_lib",
        "@io_istio_proxy//src/envoy/tcp/forward_downstream_sni:config_lib",
        "@io_istio_proxy//src/envoy/tcp/metadata_exchange:config_lib",
        "@io_istio_proxy//src/envoy/tcp/sni_verifier:config_lib",
        "@io_istio_proxy//src/envoy/tcp/tcp_cluster_rewrite:config_lib",
        "@envoy//source/exe:envoy_main_entry_lib",
    ],
)
