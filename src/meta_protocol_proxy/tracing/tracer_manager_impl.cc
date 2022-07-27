#include "src/meta_protocol_proxy/tracing/tracer_manager_impl.h"

#include "source/common/config/utility.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Tracing {

MetaProtocolTracerManagerImpl::MetaProtocolTracerManagerImpl(
    Server::Configuration::TracerFactoryContextPtr factory_context)
    : factory_context_(std::move(factory_context)) {}

MetaProtocolTracerSharedPtr MetaProtocolTracerManagerImpl::getOrCreateMetaProtocolTracer(
    const envoy::config::trace::v3::Tracing_Http* config) {
  if (!config) {
    return null_tracer_;
  }

  const auto cache_key = MessageUtil::hash(*config);
  const auto it = meta_protocol_tracers_.find(cache_key);
  if (it != meta_protocol_tracers_.end()) {
    auto meta_protocol_tracer = it->second.lock();
    if (meta_protocol_tracer) { // MetaProtocolTracer might have been released since it's a weak
                                // reference
      return meta_protocol_tracer;
    }
  }

  // Free memory held by expired weak references.
  //
  // Given that:
  //
  // * MetaProtocolTracer is obtained only once per listener lifecycle
  // * in a typical case, all listeners will have identical tracing configuration and, consequently,
  //   will share the same MetaProtocolTracer instance
  // * amount of memory held by an expired weak reference is minimal
  //
  // it seems reasonable to avoid introducing an external sweeper and only reclaim memory at
  // the moment when a new MetaProtocolTracer instance is about to be created.
  removeExpiredCacheEntries();

  // Initialize a new tracer.
  ENVOY_LOG(info, "instantiating a new tracer: {}", config->name());

  // Now see if there is a factory that will accept the config.
  auto& factory =
      Envoy::Config::Utility::getAndCheckFactory<Server::Configuration::TracerFactory>(*config);
  ProtobufTypes::MessagePtr message = Envoy::Config::Utility::translateToFactoryConfig(
      *config, factory_context_->messageValidationVisitor(), factory);

  MetaProtocolTracerSharedPtr meta_protocol_tracer = std::make_shared<MetaProtocolTracerImpl>(
      factory.createTracerDriver(*message, *factory_context_),
      factory_context_->serverFactoryContext().localInfo());
  meta_protocol_tracers_.emplace(cache_key, meta_protocol_tracer); // cache a weak reference
  return meta_protocol_tracer;
}

void MetaProtocolTracerManagerImpl::removeExpiredCacheEntries() {
  absl::erase_if(meta_protocol_tracers_,
                 [](const std::pair<const std::size_t, std::weak_ptr<MetaProtocolTracer>>& entry) {
                   return entry.second.expired();
                 });
}

} // namespace Tracing
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
