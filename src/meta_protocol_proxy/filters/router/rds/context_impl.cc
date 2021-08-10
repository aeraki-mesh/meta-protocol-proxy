#include "src/meta_protocol_proxy/filters/router/rds/context_impl.h"

namespace Envoy {
namespace MetaProtocolProxy {
namespace Router {

ContextImpl::ContextImpl(Stats::SymbolTable& symbol_table)
    : stat_names_(symbol_table), virtual_cluster_stat_names_(symbol_table) {}

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace Envoy
