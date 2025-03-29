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

void MetadataImpl::put(std::string key, std::any value) {
  properties_[key] = value;
}

AnyOptConstRef MetadataImpl::getByKey(std::string key) const {
  auto it = properties_.find(key);
  if (it != properties_.end()) {
    return OptRef<const std::any>(it->second);
  }
  return OptRef<const std::any>();
};

void MetadataImpl::putString(std::string key, std::string value) {
  this->put(key, value);
  auto lowcase_key = Http::LowerCaseString(key);
  headers_->remove(lowcase_key);
  headers_->addCopy(lowcase_key, value);
  response_headers_->remove(lowcase_key);
  response_headers_->addCopy(lowcase_key, value);
};

void MetadataImpl::removeString(std::string key) {
  properties_.erase(key);
  auto lowcase_key = Http::LowerCaseString(key);
  headers_->remove(lowcase_key);
  response_headers_->remove(lowcase_key);
};


std::string MetadataImpl::getString(std::string key) const {
  auto value = this->getByKey(key);
  if (value.has_value()) {
    return std::any_cast<std::string>(value.ref());
  }
  return "";
};

bool MetadataImpl::getBool(std::string key) const {
  auto value = this->getByKey(key);
  if (value.has_value()) {
    return std::any_cast<bool>(value.ref());
  }
  return false;
};

uint32_t MetadataImpl::getUint32(std::string key) const {
  auto value = this->getByKey(key);
  if (value.has_value()) {
    return std::any_cast<uint32_t>(value.ref());
  }
  return 0;
};

MetadataSharedPtr MetadataImpl::clone() const {
  auto copy = std::make_shared<MetadataImpl>();
  copy->originMessage().add(origin_message_);
  copy->setMessageType(getMessageType());
  copy->setResponseStatus(getResponseStatus());
  copy->setBodySize(getBodySize());
  copy->setHeaderSize(getHeaderSize());
  copy->setRequestId(getRequestId());
  copy->setStreamId(getStreamId());

  for (const auto& [key, value] : properties_) {
    copy->put(key, value);
  }
  return copy;
};

// Tracing::TraceContext
void MetadataImpl::forEach(Envoy::Tracing::TraceContext::IterateCallback callback) const {
  for (const auto& [key, value] : properties_) {
    if (value.type() == typeid(std::string)) {
      if (!callback(key, std::any_cast<std::string>(value))) {
        break;
      }
    }
  }
};

absl::optional<absl::string_view> MetadataImpl::get(absl::string_view key) const {
  // TODO use string_view instead of string
  auto val = getStringPointer(std::string{key.data(), key.length()});
  if (val && *val != "") {
    return absl::string_view{*val};
  }
  return {};
};

const std::string* MetadataImpl::getStringPointer(std::string key) const {
  auto value = this->getByKey(key);
  if (value.has_value()) {
    return std::any_cast<std::string>(value.ptr());
  }
  return nullptr;
};

void MetadataImpl::set(absl::string_view key, absl::string_view val) {
  putString(std::string(key.data(), key.length()), std::string(val.data(), val.length()));
}

void MetadataImpl::remove(absl::string_view key) {
  removeString(std::string(key.data(), key.length()));
}

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy