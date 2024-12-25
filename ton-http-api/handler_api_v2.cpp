#include "handler_api_v2.h"

#include "tl/tl_json.h"
#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api_json.h"
#include "td/utils/JsonBuilder.h"
#include "userver/components/component_context.hpp"
#include "userver/http/common_headers.hpp"
#include "openapi/openapi_page.hpp"

namespace ton_http::handlers {
template <typename IntType>
std::optional<IntType> stringToInt(const std::string& str) {
  static_assert(std::is_integral<IntType>::value, "Template parameter must be an integral type");
  try {
    if constexpr (std::is_same<IntType, int>::value) {
      return static_cast<IntType>(std::stoi(str));
    } else if constexpr (std::is_same<IntType, long>::value) {
      return static_cast<IntType>(std::stol(str));
    } else if constexpr (std::is_same<IntType, long long>::value) {
      return static_cast<IntType>(std::stoll(str));
    } else if constexpr (std::is_same<IntType, unsigned long>::value) {
      return static_cast<IntType>(std::stoul(str));
    } else if constexpr (std::is_same<IntType, unsigned long long>::value) {
      return static_cast<IntType>(std::stoull(str));
    } else if constexpr (std::is_same<IntType, unsigned int>::value) {
      return static_cast<IntType>(std::stoul(str));
    } else if constexpr (std::is_same<IntType, short>::value) {
      return static_cast<IntType>(std::stoi(str));
    } else if constexpr (std::is_same<IntType, unsigned short>::value) {
      return static_cast<IntType>(std::stoul(str));
    } else {
      throw std::invalid_argument("Unsupported integer type");
    }
  } catch(...) {
    return std::nullopt;
  }
}

std::string ApiV2Handler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request, userver::server::request::RequestContext& context
) const {
  auto& ton_api_method = request.GetPathArg("ton_api_method");
  if ((ton_api_method == "index.html") || ton_api_method.empty()) {
    request.GetHttpResponse().SetHeader(userver::http::headers::kContentType, "text/html; charset=utf-8");
    return openapi::GetOpenApiPage();
  }
  if (ton_api_method == "openapi.json") {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
    return openapi::GetOpenApiJson();
  }

  // call method
  auto res = HandleTonlibRequest(ton_api_method, request);

  // prepare response
  request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
  auto response = userver::formats::json::ValueBuilder();
  response["ok"] = res.is_ok;
  if (res.is_ok) {
    if (res.result) {
      response["result"] = userver::formats::json::FromString(td::json_encode<td::string>(td::ToJson(res.result)));
    } else if (res.result_str.has_value()) {
      response["result"] = userver::formats::json::FromString(res.result_str.value());
    } else {
      response["ok"] = false;
      response["error"] = "empty response";
    }
  } else {
    response["error"] = res.error->message().str();
    if (auto code = res.error->code(); code) {
      response["code"] = code;
    }
  }
  return userver::formats::json::ToString(response.ExtractValue());
}
ApiV2Handler::ApiV2Handler(
    const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context
) :
    HttpHandlerBase(config, context), tonlib_component_(context.FindComponent<core::TonlibComponent>()) {
}
core::TonlibWorkerResponse ApiV2Handler::HandleTonlibRequest(const std::string& ton_api_method,  const userver::server::http::HttpRequest& request) const {
  if (ton_api_method == "getAddressInformation") {
    auto address = request.GetArg("address");
    auto seqno = stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }

    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getAddressInformation, address, std::move(res));
  }

  if (ton_api_method == "getExtendedAddressInformation") {
    auto address = request.GetArg("address");
    auto seqno = stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }

    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getExtendedAddressInformation, address, seqno);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  if (ton_api_method == "getWalletInformation") {
    auto address = request.GetArg("address");
    auto seqno = stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getWalletInformation, address, std::move(res));
  }

  if (ton_api_method == "getAddressBalance") {
    auto address = request.GetArg("address");
    auto seqno = stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getAddressBalance, address, std::move(res));
  }

  if (ton_api_method == "getAddressState") {
    auto address = request.GetArg("address");
    auto seqno = stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getAddressState, address, std::move(res));
  }

  if (ton_api_method == "detectAddress") {
    auto address = request.GetArg("address");
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::detectAddress, address);
    auto res_str = res.move_as_ok().to_json_string();
    return core::TonlibWorkerResponse::from_result_string(res_str);
  }

  if (ton_api_method == "getMasterchainInfo") {
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getMasterchainInfo);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  if (ton_api_method == "lookupBlock") {
    auto workchain = stringToInt<ton::WorkchainId>(request.GetArg("workchain"));
    auto shard = stringToInt<ton::ShardId>(request.GetArg("shard"));
    auto seqno = stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto lt = stringToInt<ton::LogicalTime>(request.GetArg("lt"));
    auto unixtime = stringToInt<ton::UnixTime>(request.GetArg("unixtime"));

    if (!workchain.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required");
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::lookupBlock, workchain.value(), shard.value(), seqno, lt, unixtime);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  if (ton_api_method == "getMasterchainBlockSignatures") {
    auto seqno = stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (!seqno.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("seqno is required");
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getMasterchainBlockSignatures, seqno.value());\
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  return {false, nullptr, std::nullopt, td::Status::Error(404, "method not found")};
}

}  // namespace ton_http::handlers
