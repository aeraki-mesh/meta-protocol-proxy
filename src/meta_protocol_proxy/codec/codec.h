#pragma once

#include <any>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/common/optref.h"
#include "envoy/common/pure.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

enum class MessageType {
  Request = 0,
  Response = 1,
  Oneway = 2,
  Heartbeat = 3,
  Error = 4,
};

enum class ResponseStatus {
  Ok = 0,
  Error = 1,
};

using AnyOptConstRef = OptRef<const std::any>;
class Properties {
public:
  virtual ~Properties() = default;

  /**
   * Put a any type key:value pair in the metadata.
   * Please note that the key:value pair put by this function will not be used for routing.
   * Use putString function instead if the decoded key:value pair is intended for routing.
   * @param key
   * @param value
   */
  virtual void put(std::string key, std::any value) PURE;

  /**
   * Get the value by key from the metadata.
   * @param key
   * @return
   */
  virtual AnyOptConstRef get(std::string key) const PURE;

  /**
   * Put a string key:value pair in the metadata, the stored value will be used for routing.
   * @param key
   * @param value
   */
  virtual void putString(std::string key, std::string value) PURE;

  /**
   * Get a string value from the metadata.
   * @param key
   * @return
   */
  virtual std::string getString(std::string key) const PURE;

  /**
   * Get a bool value from the metadata.
   * @param key
   * @return
   */
  virtual bool getBool(std::string key) const PURE;

  /**
   * Get a uint32 value from the metadata.
   * @param key
   * @return
   */
  virtual uint32_t getUint32(std::string key) const PURE;
};

class Metadata : public Properties {
public:
  virtual ~Metadata() = default;

  virtual void setOriginMessage(Buffer::Instance&) PURE;
  virtual Buffer::Instance& getOriginMessage() PURE;
  virtual void setMessageType(MessageType messageType) PURE;
  virtual MessageType getMessageType() const PURE;
  virtual void setResponseStatus(ResponseStatus responseStatus) PURE;
  virtual ResponseStatus getResponseStatus() const PURE;
  virtual void setRequestId(uint64_t requestId) PURE;
  virtual uint64_t getRequestId() const PURE;
  virtual size_t getMessageSize() const PURE;
  virtual void setHeaderSize(size_t headerSize) PURE;
  virtual size_t getHeaderSize() const PURE;
  virtual void setBodySize(size_t bodySize) PURE;
  virtual size_t getBodySize() const PURE;
};
using MetadataSharedPtr = std::shared_ptr<Metadata>;

class Mutation : public Properties {
public:
  virtual ~Mutation() = default;
};
using MutationSharedPtr = std::shared_ptr<Mutation>;

enum class DecodeStatus {
  WaitForData = 0,
  Done = 1,
};

enum class ErrorType {
  RouteNotFound = 0,
  ClusterNotFound = 1,
  NoHealthyUpstream = 2,
  BadResponse = 3,
  Unspecified = 4,
  OverLimit = 5,
};

struct Error {
  ErrorType type;
  std::string message;
};

/**
 * Codec is used to decode and encode messages of a specific protocol built on top of MetaProtocol.
 */
class Codec {
public:
  virtual ~Codec() = default;

  /*
   * decodes the protocol message.
   *
   * @param buffer the currently buffered data.
   * @param metadata saves the meta data of the current message.
   * @return DecodeStatus::DONE if a complete message was successfully consumed,
   * DecodeStatus::WaitForData if more data is required.
   * @throws EnvoyException if the data is not valid for this protocol.
   */
  virtual DecodeStatus decode(Buffer::Instance& buffer, Metadata& metadata) PURE;

  /*
   * encodes the protocol message.
   *
   * @param metadata the meta data produced in the decoding phase.
   * @param mutation the mutation that needs to be encoded to the message.
   * @param buffer save the encoded message.
   * @throws EnvoyException if the metadata or mutation is not valid for this protocol.
   */
  virtual void encode(const Metadata& metadata, const Mutation& mutation,
                      Buffer::Instance& buffer) PURE;

  /*
   * encodes an error message. The encoded error message is used for local reply, for example, envoy
   * can't find the specified cluster, or there is no healthy endpoint.
   *
   * @param metadata the meta data produced in the decoding phase.
   * @param error the error that needs to be encoded in the message.
   * @param buffer save the encoded message.
   * @throws EnvoyException if the metadata is not valid for this protocol.
   */
  virtual void onError(const Metadata& metadata, const Error& error, Buffer::Instance& buffer) PURE;
};

using CodecPtr = std::unique_ptr<Codec>;

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
