#include <any>
#include <memory>

#include "envoy/buffer/buffer.h"
#include "envoy/common/optref.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/codec_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

void PropertiesImpl::put(std::string key, std::any value) { map_->insert({key, value}); }

AnyOptConstRef PropertiesImpl::get(std::string key) const {
  auto it = map_->find(key);
  if (it != map_->end()) {
    return OptRef<const std::any>(it->second);
  }
  return OptRef<const std::any>();
}

void PropertiesImpl::putString(std::string key, std::string value) { this->put(key, value); }

std::string PropertiesImpl::getString(std::string key) const {
  auto value = this->get(key);
  if (value.has_value()) {
    return std::any_cast<std::string>(value.ref());
  }
  return "";
}
bool PropertiesImpl::getBool(std::string key) const {
  auto value = this->get(key);
  if (value.has_value()) {
    return std::any_cast<bool>(value.ref());
  }
  return false;
}

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
