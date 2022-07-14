#pragma once

#include <any>
#include <memory>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/http/header_map.h"
#include "envoy/common/optref.h"
#include "envoy/common/pure.h"
#include "source/common/buffer/buffer_impl.h"
#include "source/common/http/header_map_impl.h"

#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

class PropertiesImpl : public Properties {
public:
  PropertiesImpl() : map_(std::make_unique<std::map<std::string, std::any>>()){};
  ~PropertiesImpl() override = default;

  void put(std::string key, std::any value) override;
  AnyOptConstRef get(std::string key) const override;
  void putString(std::string key, std::string value) override;
  std::string getString(std::string key) const override;
  bool getBool(std::string key) const override;
  uint32_t getUint32(std::string key) const override;

private:
  std::unique_ptr<std::map<std::string, std::any>> map_;
};

class MetadataImpl : public Metadata {
public:
  MetadataImpl() { headers_ = Http::RequestHeaderMapImpl::create(); };
  ~MetadataImpl() = default;

  void put(std::string key, std::any value) override { properties_.put(key, value); };
  AnyOptConstRef get(std::string key) const override { return properties_.get(key); };
  void putString(std::string key, std::string value) override {
    this->put(key, value);
    auto lowcase_key = Http::LowerCaseString(key);
    headers_->remove(lowcase_key);
    headers_->addCopy(lowcase_key, value);
  };
  std::string getString(std::string key) const override { return properties_.getString(key); };
  bool getBool(std::string key) const override { return properties_.getBool(key); };
  uint32_t getUint32(std::string key) const override { return properties_.getUint32(key); };

  void setOriginMessage(Buffer::Instance& originMessage) override {
    origin_message_ = originMessage;
  };
  Buffer::Instance& getOriginMessage() override { return origin_message_; };
  void setMessageType(MessageType messageType) override { message_type_ = messageType; };
  MessageType getMessageType() const override { return message_type_; };
  void setResponseStatus(ResponseStatus responseStatus) override {
    response_status_ = responseStatus;
  };
  ResponseStatus getResponseStatus() const override { return response_status_; };
  void setRequestId(uint64_t requestId) override { request_id_ = requestId; };
  uint64_t getRequestId() const override { return request_id_; };
  void setStreamId(uint64_t streamId) override { stream_id_ = streamId; };
  uint64_t getStreamId() const override { return stream_id_; };
  size_t getMessageSize() const override { return header_size_ + body_size_; }
  void setHeaderSize(size_t headerSize) override { header_size_ = headerSize; };
  size_t getHeaderSize() const override { return header_size_; };
  void setBodySize(size_t bodySize) override { body_size_ = bodySize; };
  size_t getBodySize() const override { return body_size_; };
  MetadataSharedPtr clone() const override{
    auto copy = std::make_shared<MetadataImpl>();

    Buffer::OwnedImpl originalMessage;
    originalMessage.add(origin_message_);
    copy->setOriginMessage(originalMessage);

    copy->setMessageType(getMessageType());
    copy->setResponseStatus(getResponseStatus());
    copy->setBodySize(getBodySize());
    copy->setHeaderSize(getHeaderSize());
    copy->setRequestId(getRequestId());
    copy->setStreamId(getStreamId());

    //TODO copy headers
    return copy;
  };
  const Http::HeaderMap& getHeaders() const { return *headers_; }

private:
  PropertiesImpl properties_;
  Buffer::OwnedImpl origin_message_;
  MessageType message_type_{MessageType::Request};
  ResponseStatus response_status_{ResponseStatus::Ok};
  uint64_t request_id_{0};
  uint64_t stream_id_{0};
  size_t header_size_{0};
  size_t body_size_{0};
  // Reuse the HeaderMatcher API and related tools provided by Envoy to match the route
  Http::HeaderMapPtr headers_;
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
