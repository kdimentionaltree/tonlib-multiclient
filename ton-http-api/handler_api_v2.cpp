#include "handler_api_v2.h"

#include <ranges>

#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api_json.h"
#include "http/http.h"
#include "openapi/openapi_page.hpp"
#include "td/utils/JsonBuilder.h"
#include "tl/tl_json.h"
#include "userver/cache/expirable_lru_cache.hpp"
#include "userver/cache/lru_cache_component_base.hpp"
#include "userver/components/component_context.hpp"
#include "userver/http/common_headers.hpp"
#include "userver/logging/component.hpp"
#include "userver/logging/log.hpp"
#include "utils.hpp"

namespace ton_http::handlers {
userver::formats::json::Value ApiV2Handler::build_json_response(const core::TonlibWorkerResponse& res) const {
  userver::formats::json::ValueBuilder response;
  response["ok"] = res.is_ok;
  if (res.is_ok) {
    if (res.result) {
      response["result"] = userver::formats::json::FromString(td::json_encode<td::string>(td::ToJson(res.result)));
    } else if (res.result_str.has_value()) {
      try {
        response["result"] = userver::formats::json::FromString(res.result_str.value());
      } catch (const std::exception& e) {
        LOG_DEBUG_TO(*logger_) << "Failed to parse result_str: " << e.what() << " value: " << res.result_str.value();
        response["result"] = res.result_str.value();
      }
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
  if (res.session) {
    response["@extra"] = res.session->to_string();
  }
  return response.ExtractValue();
}
void ApiV2Handler::log_request(
    const userver::server::http::HttpRequest& request,
    const TonlibApiRequest& req,
    const core::TonlibWorkerResponse& res,
    const std::string& response_body
) const {
  userver::logging::LogExtra log_extra;
  log_extra.Extend("http_method", req.http_method);
  log_extra.Extend("ton_api_method", req.ton_api_method);
  log_extra.Extend("url", request.GetUrl());

  auto code = res.is_ok ? 200 : res.error->code();
  if (code == 0) {
    code = 500;
  }
  if (code == -3) {
    code = 500;
  }

  log_extra.Extend("status_code", res.is_ok ? 200 : res.error->code());
  log_extra.Extend("status_code_fixed", code);
  if (is_log_required(req, res)) {
    userver::formats::json::ValueBuilder request_params;
    for (auto& [k, v] : req.args) {
      std::stringstream ss;
      bool is_first = true;
      for (auto& i : v) {
        if (is_first) {
          is_first = false;
        } else {
          ss << ",";
        }
        ss << i;
      }
      request_params[k] = ss.str();
    }
    log_extra.Extend("request_params", ToString(request_params.ExtractValue()));
    log_extra.Extend("response", response_body);
    log_extra.Extend("body", request.RequestBody());
    LOG_WARNING_TO(*logger_) << log_extra;
  } else {
    LOG_INFO_TO(*logger_) << log_extra;
  }
}

std::vector<std::string> ApiV2Handler::parse_request_body_item(const userver::formats::json::Value& value, int parse_array_depth) const {
  if (value.IsArray()) {
    if (parse_array_depth == 0) {
      return {ToString(value)};
    }
    std::vector<std::string> result;
    for (auto item = value.begin(); item != value.end(); ++item) {
      auto loc = parse_request_body_item(*item, std::max(parse_array_depth - 1, 0));
      result.push_back(loc[0]);
    }
    return result;
  }
  if (value.IsObject()) {
    return {ToString(value)};
  }
  if (value.IsString()) {
    return {value.As<std::string>()};
  }
  if (value.IsInt()) {
    return {std::to_string(value.As<int>())};
  }
  if (value.IsBool()) {
    return {std::to_string(value.As<bool>())};
  }
  return {};
}


std::string ApiV2Handler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request, userver::server::request::RequestContext& context
) const {
  TonlibApiRequest req;
  req.http_method = request.GetMethodStr();
  {
    const auto& debug_request_header = request.GetHeader("X-Debug-Request");
    auto debug_request = utils::stringToBool(debug_request_header);
    req.is_debug_request = debug_request.has_value() && debug_request.value();
  }
  auto ton_api_method_case_sensitive = request.GetPathArg("ton_api_method");
  std::ranges::copy(std::views::transform(ton_api_method_case_sensitive, ::tolower), std::back_inserter(req.ton_api_method));
  for (auto& name : request.ArgNames()) {
    req.SetArgVector(name, request.GetArgVector(name));
  }
  if (!request.RequestBody().empty()) {
    try {
      auto body = userver::formats::json::FromString(request.RequestBody());
      for (auto it = body.begin(); it != body.end(); ++it) {
        auto value = parse_request_body_item(*it, 1);
        req.SetArgVector(it.GetName(), value);
        // std::stringstream ss1;
        // for (auto& i : value) { ss1 << i << ";"; }
        // LOG_ERROR_TO(*logger_) << "arg: " << it.GetName() << " size: " << value.size() << " value: " << ss1.str();
      }
    } catch (const userver::formats::json::ParseException& e) {
      request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
      auto response = userver::formats::json::ValueBuilder();
      response["ok"] = false;
      response["code"] = 422;
      response["error"] = e.what();
      return userver::formats::json::ToString(response.ExtractValue());
    }
  }

  // handle request
  if (req.ton_api_method == "index.html" || req.ton_api_method.empty()) {
    request.GetHttpResponse().SetHeader(userver::http::headers::kContentType, "text/html; charset=utf-8");
    return openapi::GetOpenApiPage();
  }
  if (req.ton_api_method == "openapi.json") {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
    return openapi::GetOpenApiJson();
  }

  if (req.ton_api_method == "jsonrpc") {
    TonlibApiRequest jsonrpc_req;
    jsonrpc_req.http_method = req.http_method;
    auto api_method_case_sensitive = req.GetArg("method");
    std::ranges::copy(std::views::transform(api_method_case_sensitive, ::tolower), std::back_inserter(jsonrpc_req.ton_api_method));

    auto body = userver::formats::json::FromString(req.GetArg("params"));
    for (auto it = body.begin(); it != body.end(); ++it) {
      auto value = parse_request_body_item(*it, 1);
      jsonrpc_req.SetArgVector(it.GetName(), value);
      // std::stringstream ss1;
      // for (auto& i : value) { ss1 << i << ";"; }
      // LOG_ERROR_TO(*logger_) << "arg: " << it.GetName() << " size: " << value.size() << " value: " << ss1.str();
    }
    req = std::move(jsonrpc_req);
  }

  // call method
  userver::formats::json::Value response;
  auto cached_response = cache_component_.Get(req);
  if (cached_response.has_value()) {
    response = std::move(cached_response.value());
    auto response_builder = userver::formats::json::ValueBuilder(response);
    response_builder["@extra"] = response["@extra"].As<std::string>("") + ":c";
    auto response_str = userver::formats::json::ToString(response_builder.ExtractValue());
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
    request.GetHttpResponse().SetStatus(userver::server::http::HttpStatus::kOk);
    return response_str;
  }
  auto res = HandleTonlibRequest(req);

  // prepare response
  request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
  auto code = res.is_ok ? 200 : res.error->code();
  if (code == 0) { code = 500; }
  if (code == -3) { code = 500; }
  request.GetHttpResponse().SetStatus(static_cast<userver::server::http::HttpStatus>(code));
  response = build_json_response(res);
  auto response_str = userver::formats::json::ToString(response);
  log_request(request, req, res, response_str);
  if (res.is_ok && res.cache_ttl > 0) {
    cache_component_.Put(req, response);
  }
  return response_str;
}
ApiV2Handler::ApiV2Handler(
    const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context
) :
  HttpHandlerBase(config, context),
  tonlib_component_(context.FindComponent<core::TonlibComponent>()),
  cache_component_(context.FindComponent<cache::CacheApiV2Component>()),
  logger_(context.FindComponent<userver::components::Logging>().GetLogger("api-v2")) {
}
core::TonlibWorkerResponse ApiV2Handler::HandleTonlibRequest(const TonlibApiRequest& request) const {
  std::string ton_api_method;
  std::ranges::copy(std::views::transform(request.ton_api_method, ::tolower), std::back_inserter(ton_api_method));

  if (ton_api_method == "getaddressinformation") {
    auto address = request.GetArg("address");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422, nullptr);
    }

    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno, nullptr);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getAddressInformation, address, std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "getextendedaddressinformation") {
    auto address = request.GetArg("address");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422, nullptr);
    }

    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getExtendedAddressInformation, address, seqno, nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "getwalletinformation") {
    auto address = request.GetArg("address");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno, nullptr);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getWalletInformation, address, std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "getaddressbalance") {
    auto address = request.GetArg("address");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno, nullptr);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getAddressBalance, address, std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "getaddressstate") {
    auto address = request.GetArg("address");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno, nullptr);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getAddressState, address, std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "detectaddress") {
    auto address = request.GetArg("address");
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::detectAddress, address, nullptr);
    if (res.is_error()) {
      auto error = res.move_as_error();
      return core::TonlibWorkerResponse::from_error_string(error.to_string(), error.code(), std::move(session));
    }
    auto res_str = res.move_as_ok().to_json_string();
    return core::TonlibWorkerResponse::from_result_string(res_str, std::move(session)).Cachable();
  }

  if (ton_api_method == "gettokendata") {
    auto address = request.GetArg("address");
    auto skip_verification = utils::stringToBool(request.GetArg("skip_verification"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto archival = utils::stringToBool(request.GetArg("archival"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422, nullptr);
    }
    if (!skip_verification.has_value()) {
      skip_verification = false;
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getTokenData, address, skip_verification.value(), seqno, archival, nullptr);
    if (res.is_error()) {
      auto error = res.move_as_error();
      return core::TonlibWorkerResponse::from_error_string(error.to_string(), error.code(), std::move(session));
    }
    auto res_str = res.move_as_ok()->to_json_string();
    return core::TonlibWorkerResponse::from_result_string(res_str, std::move(session)).Cachable();
  }

  if (ton_api_method == "detecthash") {
    auto hash = request.GetArg("hash");
    if (hash.empty()) {
      return core::TonlibWorkerResponse::from_error_string("hash is required", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::detectHash, hash, nullptr);
    if (res.is_error()) {
      auto error = res.move_as_error();
      return core::TonlibWorkerResponse::from_error_string(error.to_string(), error.code(), std::move(session));
    }
    auto res_str = res.move_as_ok().to_json_string();
    return core::TonlibWorkerResponse::from_result_string(res_str, std::move(session)).Cachable();
  }

  if (ton_api_method == "getmasterchaininfo") {
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getMasterchainInfo, nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }
  if (ton_api_method == "getconsensusblock") {
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getConsensusBlock, nullptr);
    if (res.is_error()) {
      auto error = res.move_as_error();
      return core::TonlibWorkerResponse::from_error_string(error.to_string(), error.code(), std::move(session));
    }
    auto res_str = res.move_as_ok().to_json_string();
    return core::TonlibWorkerResponse::from_result_string(res_str, std::move(session)).Cachable();
  }

  if (ton_api_method == "getmasterchainblocksignatures") {
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (!seqno.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("seqno is required", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getMasterchainBlockSignatures, seqno.value(), nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "getshardblockproof") {
    auto workchain = utils::stringToInt<ton::WorkchainId>(request.GetArg("workchain"));
    auto shard = utils::stringToInt<ton::ShardId>(request.GetArg("shard"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto from_seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("from_seqno"));

    if (!workchain.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422, nullptr);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required", 422, nullptr);
    }
    if (!seqno.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("seqno is required", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getShardBlockProof,
      workchain.value(), shard.value(), seqno.value(), from_seqno, nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "lookupblock") {
    auto workchain = utils::stringToInt<ton::WorkchainId>(request.GetArg("workchain"));
    auto shard = utils::stringToInt<ton::ShardId>(request.GetArg("shard"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("lt"));
    auto unixtime = utils::stringToInt<ton::UnixTime>(request.GetArg("unixtime"));

    if (!workchain.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422, nullptr);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::lookupBlock,
      workchain.value(), shard.value(), seqno, lt, unixtime, nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "getshards" || ton_api_method == "shards") {
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("lt"));
    auto unixtime = utils::stringToInt<ton::UnixTime>(request.GetArg("unixtime"));

    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getShards, seqno, lt, unixtime, nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "getblockheader") {
    auto workchain = utils::stringToInt<ton::WorkchainId>(request.GetArg("workchain"));
    auto shard = utils::stringToInt<ton::ShardId>(request.GetArg("shard"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto root_hash = utils::stringToHash(request.GetArg("root_hash"));
    auto file_hash = utils::stringToHash(request.GetArg("file_hash"));

    if (!workchain.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422, nullptr);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required", 422, nullptr);
    }
    if (!seqno.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("seqno is required", 422, nullptr);
    }
    if (!root_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse root_hash", 422, nullptr);
    }
    if (!file_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse file_hash", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getBlockHeader,
      workchain.value(), shard.value(), seqno.value(), root_hash.value(), file_hash.value(), nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "getoutmsgqueuesizes") {
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getOutMsgQueueSizes, nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "lookupblock") {
    auto workchain = utils::stringToInt<ton::WorkchainId>(request.GetArg("workchain"));
    auto shard = utils::stringToInt<ton::ShardId>(request.GetArg("shard"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("lt"));
    auto unixtime = utils::stringToInt<ton::UnixTime>(request.GetArg("unixtime"));

    if (!workchain.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422, nullptr);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::lookupBlock,
      workchain.value(), shard.value(), seqno, lt, unixtime, nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "getblocktransactions" || ton_api_method == "getblocktransactionsext") {
    auto workchain = utils::stringToInt<ton::WorkchainId>(request.GetArg("workchain"));
    auto shard = utils::stringToInt<ton::ShardId>(request.GetArg("shard"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto root_hash = utils::stringToHash(request.GetArg("root_hash"));
    auto file_hash = utils::stringToHash(request.GetArg("file_hash"));
    auto after_lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("after_lt"));
    auto after_hash = utils::stringToHash(request.GetArg("after_hash"));
    auto count = utils::stringToInt<std::int32_t>(request.GetArg("count"));

    if (!workchain.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422, nullptr);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required", 422, nullptr);
    }
    if (!seqno.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("seqno is required", 422, nullptr);
    }
    if (!root_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse root_hash", 422, nullptr);
    }
    if (!file_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse file_hash", 422, nullptr);
    }
    if (!after_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse after_hash", 422, nullptr);
    }
    if (!count.has_value()) {
      count = 40;
    }
    std::optional<bool> archival = std::nullopt;
    if (ton_api_method == "getblocktransactions") {
      auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getBlockTransactions,
      workchain.value(), shard.value(), seqno.value(), count.value(), root_hash.value(), file_hash.value(), after_lt, after_hash.value(), archival, nullptr);

      return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getBlockTransactions, std::move(res), std::move(session));
    } else if (ton_api_method == "getblocktransactionsext") {
      auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getBlockTransactionsExt,
      workchain.value(), shard.value(), seqno.value(), count.value(), root_hash.value(), file_hash.value(), after_lt, after_hash.value(), archival, nullptr);

      return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getBlockTransactionsExt, std::move(res), std::move(session)).Cachable();
    }
  }

  if (ton_api_method == "gettransactions" || ton_api_method == "gettransactionsv2") {
    auto address = request.GetArg("address");
    auto limit = utils::stringToInt<std::int32_t>(request.GetArg("limit"));
    auto count = utils::stringToInt<std::int32_t>(request.GetArg("count"));
    auto chunk_size = utils::stringToInt<std::int32_t>(request.GetArg("chunk_size"));
    auto from_transaction_lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("lt"));
    auto from_transaction_hash = utils::stringToHash(request.GetArg("hash"));
    auto to_transaction_lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("to_lt"));
    auto archival = utils::stringToBool(request.GetArg("archival"));
    bool try_decode_messages = true;

    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422, nullptr);
    }
    if (!from_transaction_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse from_transaction_hash", 422, nullptr);
    }


    if (limit.has_value()) {
      count = limit.value();
    }
    if (!count.has_value()) {
      count = 10;
    }
    if (!to_transaction_lt.has_value()) {
      to_transaction_lt = 0;
    }
    if (!chunk_size.has_value()) {
      chunk_size = 30;
    }

    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getTransactions,
      address,
      from_transaction_lt,
      from_transaction_hash.value(),
      to_transaction_lt.value(),
      count.value(),
      chunk_size.value(),
      try_decode_messages,
      archival,
      nullptr
    );
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getTransactions, std::move(res), ton_api_method == "gettransactionsv2", false, std::move(session)).Cachable();
  }

  if (ton_api_method == "trylocatetx" || ton_api_method == "trylocateresulttx") {
    auto source = request.GetArg("source");
    auto destination = request.GetArg("destination");
    auto created_lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("created_lt"));

    if (source.empty()) {
      return core::TonlibWorkerResponse::from_error_string("source is required", 422, nullptr);
    }
    if (destination.empty()) {
      return core::TonlibWorkerResponse::from_error_string("destination is required", 422, nullptr);
    }
    if (!created_lt.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("created_lt is required", 422, nullptr);
    }

    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::tryLocateTransactionByIncomingMessage, source, destination, created_lt.value(), nullptr);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getTransactions, std::move(res), false, true, std::move(session)).Cachable();
  }
  if (ton_api_method == "trylocatesourcetx") {
    auto source = request.GetArg("source");
    auto destination = request.GetArg("destination");
    auto created_lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("created_lt"));

    if (source.empty()) {
      return core::TonlibWorkerResponse::from_error_string("source is required", 422, nullptr);
    }
    if (destination.empty()) {
      return core::TonlibWorkerResponse::from_error_string("destination is required", 422, nullptr);
    }
    if (!created_lt.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("created_lt is required", 422, nullptr);
    }

    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::tryLocateTransactionByOutgoingMessage, source, destination, created_lt.value(), nullptr);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getTransactions, std::move(res), false, true, std::move(session)).Cachable();
  }

  if (ton_api_method == "getconfigparam") {
    auto config_id = utils::stringToInt<std::int32_t>(request.GetArg("config_id"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (!config_id.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("config_id is required", 422, nullptr);
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getConfigParam, config_id.value(), seqno, nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }
  if (ton_api_method == "getconfigall") {
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getConfigAll, seqno, nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "getlibraries") {
    std::vector<std::string> libs;

    std::stringstream ss;
    for (auto &l : request.GetArgVector("libraries")) {
      auto lib = utils::stringToHash(l);
      if (!lib.has_value()) {
        return core::TonlibWorkerResponse::from_error_string("failed to parse library", 422, nullptr);
      }
      ss << lib.value() << ";";
      libs.push_back(std::move(lib.value()));
    }
    // LOG_ERROR_TO(*logger_) << "getlibraries: " << libs;
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::getLibraries, libs, nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  if (ton_api_method == "sendboc") {
    auto boc = request.GetArg("boc");
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::raw_sendMessage, boc, nullptr);
    if (res.is_ok()) {
      tonlib_component_.SendBocToExternalRequest(boc);
    }
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session));
  }

  if (ton_api_method == "sendbocreturnhash") {
    auto boc = request.GetArg("boc");
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::raw_sendMessageReturnHash, boc, nullptr);
    if (res.is_ok()) {
      tonlib_component_.SendBocToExternalRequest(boc);
    }
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session));
  }
  if (ton_api_method == "sendbocreturnhashnoerror") {
    auto boc = request.GetArg("boc");
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::raw_sendMessageReturnHash, boc, nullptr);
    auto result = core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session));
    if (!result.is_ok && result.error.has_value()) {
      result.error = td::Status::Error(200, result.error->message());
    } else {
      tonlib_component_.SendBocToExternalRequest(boc);
    }
    return std::move(result);
  }

  if (ton_api_method == "rungetmethod") {
    auto address = request.GetArg("address");
    auto method = request.GetArg("method");
    auto stack = request.GetArgVector("stack");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto archival = utils::stringToBool(request.GetArg("archival"));

    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::runGetMethod, address, method, stack, seqno, archival, nullptr);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_runGetMethod, std::move(res), std::move(session));
  }

  if (ton_api_method == "unpackaddress") {
    auto address = request.GetArg("address");
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::unpackAddress, address, nullptr);
    if (res.is_error()) {
      auto error = res.move_as_error();
      return core::TonlibWorkerResponse::from_error_string(error.message().str(), error.code(), std::move(session));
    }
    return core::TonlibWorkerResponse::from_result_string(res.move_as_ok(), std::move(session)).Cachable();
  }

  if (ton_api_method == "packaddress") {
    auto address = request.GetArg("address");
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::packAddress, address, nullptr);
    if (res.is_error()) {
      auto error = res.move_as_error();
      return core::TonlibWorkerResponse::from_error_string(error.message().str(), error.code(), std::move(session));
    }
    return core::TonlibWorkerResponse::from_result_string(res.move_as_ok(), std::move(session)).Cachable();
  }

  if (ton_api_method == "estimatefee") {
    auto address = request.GetArg("address");
    auto body = request.GetArg("body");
    auto init_code = request.GetArg("init_code");
    auto init_data = request.GetArg("init_data");
    auto ignore_chksig = utils::stringToBool(request.GetArg("ignore_chksig"));

    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("body is required", 422, nullptr);
    }
    if (body.empty()) {
      return core::TonlibWorkerResponse::from_error_string("body is required", 422, nullptr);
    }
    if (!ignore_chksig.has_value()) {
      ignore_chksig = true;
    }
    auto [res, session] = tonlib_component_.DoRequest(&core::TonlibWorker::queryEstimateFees, address, body, init_code, init_data, ignore_chksig.value(), nullptr);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res), std::move(session)).Cachable();
  }

  return {false, nullptr, std::nullopt, td::Status::Error(404, "method not found"), nullptr};
}
bool ApiV2Handler::is_log_required(const TonlibApiRequest& request, const core::TonlibWorkerResponse& response) const {
  if (request.is_debug_request) {
    return true;
  }
  if (response.is_ok) {
    return false;
  }
  if (response.error.has_value()) {
    auto& error = response.error.value();
    auto status_code = error.code();
    if (status_code == 404) {
      return false;
    }
    if (status_code == 409) {
      return false;
    }
    if ((status_code == 500 || status_code == 0) && request.ton_api_method.starts_with("sendboc")) {
      return false;
    }
  }
  return true;
}

}  // namespace ton_http::handlers
