// Copyright (c) 2020, Tencent Inc.
// All rights reserved.

#include "src/application_protocols/trpc/metadata.h"

#include <algorithm>

namespace {

inline std::string convertCalleeToHost(std::string const& callee) {
  // tRPC的被调服务的路由名称如下:
  //  规范格式，trpc.应用名.服务名.pb的service名[.接口名]
  //  其中接口可选
  // 如果有接口的名话，需要去除，只保留到service
  if (std::count(callee.begin(), callee.end(), '.') != 4) {
    return callee;
  }

  std::string host_copy(callee);
  host_copy.resize(host_copy.find_last_of('.'));
  return host_copy; // copy elision
}

constexpr char kUserAgentValues[] = "trpc-proxy";
constexpr char kProtocolValues[] = "trpc";
constexpr char kContentTypeValues[] = "application/trpc";

} // namespace

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Trpc {

// convert trpc meta to http headers
void MessageMetadata::buildHttpHeaders() {
  http_request_headers = Http::RequestHeaderMapImpl::create();
  http_request_headers->addReference(Http::Headers::get().Method,
                                     Http::Headers::get().MethodValues.Post);
  http_request_headers->addReference(Http::Headers::get().ForwardedProto,
                                     Http::Headers::get().SchemeValues.Http);
  http_request_headers->addReference(Http::Headers::get().UserAgent, kUserAgentValues);
  http_request_headers->addReference(Http::Headers::get().Protocol, kProtocolValues);
  http_request_headers->addReference(Http::Headers::get().Scheme, kProtocolValues);
  http_request_headers->addReference(Http::Headers::get().ContentType, kContentTypeValues);

  http_request_headers->addReferenceKey(Http::Headers::get().Host,
                                        convertCalleeToHost(request_protocol.callee()));
  http_request_headers->addReferenceKey(Http::Headers::get().Path, request_protocol.func());
  http_request_headers->addReferenceKey(Http::Headers::get().RequestId,
                                        request_protocol.request_id());

  for (auto const& kv : request_protocol.trans_info()) {
    http_request_headers->addCopy(Http::LowerCaseString(kv.first), kv.second);
  }
}

} // namespace Trpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy