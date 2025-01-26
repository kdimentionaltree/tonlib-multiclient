#include "handler_api_v2.h"

#include <ranges>
#include "tl/tl_json.h"
#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api_json.h"
#include "td/utils/JsonBuilder.h"
#include "userver/components/component_context.hpp"
#include "userver/http/common_headers.hpp"
#include "openapi/openapi_page.hpp"
#include "utils.hpp"

namespace ton_http::handlers {
std::string ApiV2Handler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request, userver::server::request::RequestContext& context
) const {
  TonlibApiRequest req;
  req.http_method = request.GetMethodStr();
  req.ton_api_method = request.GetPathArg("ton_api_method");
  for (auto& name : request.ArgNames()) {
    req.SetArg(name, request.GetArg(name));
  }

  if (req.ton_api_method == "index.html" || req.ton_api_method.empty()) {
    request.GetHttpResponse().SetHeader(userver::http::headers::kContentType, "text/html; charset=utf-8");
    return openapi::GetOpenApiPage();
  }
  if (req.ton_api_method == "openapi.json") {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
    return openapi::GetOpenApiJson();
  }

  // call method
  auto res = HandleTonlibRequest(req);

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
core::TonlibWorkerResponse ApiV2Handler::HandleTonlibRequest(const TonlibApiRequest& request) const {
  std::string ton_api_method;
  std::ranges::copy(std::views::transform(request.ton_api_method, ::tolower), std::back_inserter(ton_api_method));

  if (ton_api_method == "getaddressinformation") {
    auto address = request.GetArg("address");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }

    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getAddressInformation, address, std::move(res));
  }

  if (ton_api_method == "getextendedaddressinformation") {
    auto address = request.GetArg("address");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }

    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getExtendedAddressInformation, address, seqno);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  if (ton_api_method == "getwalletinformation") {
    auto address = request.GetArg("address");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getWalletInformation, address, std::move(res));
  }

  if (ton_api_method == "getaddressbalance") {
    auto address = request.GetArg("address");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getAddressBalance, address, std::move(res));
  }

  if (ton_api_method == "getaddressstate") {
    auto address = request.GetArg("address");
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getAddressInformation, address, seqno);
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getAddressState, address, std::move(res));
  }

  if (ton_api_method == "detectaddress") {
    auto address = request.GetArg("address");
    if (address.empty()) {
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::detectAddress, address);
    if (res.is_error()) {
      return core::TonlibWorkerResponse::from_error_string(res.move_as_error().to_string());
    }
    auto res_str = res.move_as_ok().to_json_string();
    return core::TonlibWorkerResponse::from_result_string(res_str);
  }
  if (ton_api_method == "detecthash") {
    auto hash = request.GetArg("hash");
    if (hash.empty()) {
      return core::TonlibWorkerResponse::from_error_string("hash is required", 422);
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::detectHash, hash);
    if (res.is_error()) {
      return core::TonlibWorkerResponse::from_error_string(res.move_as_error().to_string());
    }
    auto res_str = res.move_as_ok().to_json_string();
    return core::TonlibWorkerResponse::from_result_string(res_str);
  }

  if (ton_api_method == "getmasterchaininfo") {
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getMasterchainInfo);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }
  if (ton_api_method == "getconsensusblock") {
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getConsensusBlock);
    if (res.is_error()) {
      return core::TonlibWorkerResponse::from_error_string(res.move_as_error().to_string());
    }
    auto res_str = res.move_as_ok().to_json_string();
    return core::TonlibWorkerResponse::from_result_string(res_str);
  }

  if (ton_api_method == "getmasterchainblocksignatures") {
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (!seqno.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("seqno is required");
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getMasterchainBlockSignatures, seqno.value());\
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  if (ton_api_method == "getshardblockproof") {
    auto workchain = utils::stringToInt<ton::WorkchainId>(request.GetArg("workchain"));
    auto shard = utils::stringToInt<ton::ShardId>(request.GetArg("shard"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto from_seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("from_seqno"));

    if (!workchain.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required");
    }
    if (!seqno.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("seqno is required");
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getShardBlockProof,
      workchain.value(), shard.value(), seqno.value(), from_seqno);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  if (ton_api_method == "lookupblock") {
    auto workchain = utils::stringToInt<ton::WorkchainId>(request.GetArg("workchain"));
    auto shard = utils::stringToInt<ton::ShardId>(request.GetArg("shard"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("lt"));
    auto unixtime = utils::stringToInt<ton::UnixTime>(request.GetArg("unixtime"));

    if (!workchain.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required");
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::lookupBlock,
      workchain.value(), shard.value(), seqno, lt, unixtime);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  if (ton_api_method == "getshards" || ton_api_method == "shards") {
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("lt"));
    auto unixtime = utils::stringToInt<ton::UnixTime>(request.GetArg("unixtime"));

    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getShards, seqno, lt, unixtime);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  if (ton_api_method == "getblockheader") {
    auto workchain = utils::stringToInt<ton::WorkchainId>(request.GetArg("workchain"));
    auto shard = utils::stringToInt<ton::ShardId>(request.GetArg("shard"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto root_hash = utils::stringToHash(request.GetArg("root_hash"));
    auto file_hash = utils::stringToHash(request.GetArg("file_hash"));

    if (!workchain.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required");
    }
    if (!seqno.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("seqno is required");
    }
    if (!root_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse root_hash");
    }
    if (!file_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse file_hash");
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getBlockHeader,
      workchain.value(), shard.value(), seqno.value(), root_hash.value(), file_hash.value());
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  if (ton_api_method == "getoutmsgqueuesizes") {
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getOutMsgQueueSizes);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  if (ton_api_method == "lookupblock") {
    auto workchain = utils::stringToInt<ton::WorkchainId>(request.GetArg("workchain"));
    auto shard = utils::stringToInt<ton::ShardId>(request.GetArg("shard"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("lt"));
    auto unixtime = utils::stringToInt<ton::UnixTime>(request.GetArg("unixtime"));

    if (!workchain.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required");
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::lookupBlock,
      workchain.value(), shard.value(), seqno, lt, unixtime);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
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
      return core::TonlibWorkerResponse::from_error_string("workchain is required", 422);
    }
    if (!shard.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("shard is required");
    }
    if (!seqno.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("seqno is required");
    }
    if (!root_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse root_hash");
    }
    if (!file_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse file_hash");
    }
    if (!after_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse after_hash");
    }
    if (!count.has_value()) {
      count = 40;
    }
    std::optional<bool> archival = std::nullopt;
    if (ton_api_method == "getblocktransactions") {
      auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getBlockTransactions,
      workchain.value(), shard.value(), seqno.value(), count.value(), root_hash.value(), file_hash.value(), after_lt, after_hash.value(), archival);

      return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getBlockTransactions, std::move(res));
    } else if (ton_api_method == "getblocktransactionsext") {
      auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getBlockTransactionsExt,
      workchain.value(), shard.value(), seqno.value(), count.value(), root_hash.value(), file_hash.value(), after_lt, after_hash.value(), archival);

      return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getBlockTransactionsExt, std::move(res));
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
      return core::TonlibWorkerResponse::from_error_string("address is required", 422);
    }
    if (!from_transaction_hash.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("failed to parse from_transaction_hash");
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

    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getTransactions,
      address,
      from_transaction_lt,
      from_transaction_hash.value(),
      to_transaction_lt.value(),
      count.value(),
      chunk_size.value(),
      try_decode_messages,
      archival
    );
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getTransactions, std::move(res), ton_api_method == "gettransactionsv2", false);
  }

  if (ton_api_method == "trylocatetx" || ton_api_method == "trylocateresulttx") {
    auto source = request.GetArg("source");
    auto destination = request.GetArg("destination");
    auto created_lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("created_lt"));

    if (source.empty()) {
      return core::TonlibWorkerResponse::from_error_string("source is required", 422);
    }
    if (destination.empty()) {
      return core::TonlibWorkerResponse::from_error_string("destination is required", 422);
    }
    if (!created_lt.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("created_lt is required", 422);
    }

    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::tryLocateTransactionByIncomingMessage, source, destination, created_lt.value());
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getTransactions, std::move(res), false, true);
  }
  if (ton_api_method == "trylocatesourcetx") {
    auto source = request.GetArg("source");
    auto destination = request.GetArg("destination");
    auto created_lt = utils::stringToInt<ton::LogicalTime>(request.GetArg("created_lt"));

    if (source.empty()) {
      return core::TonlibWorkerResponse::from_error_string("source is required", 422);
    }
    if (destination.empty()) {
      return core::TonlibWorkerResponse::from_error_string("destination is required", 422);
    }
    if (!created_lt.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("created_lt is required", 422);
    }

    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::tryLocateTransactionByOutgoingMessage, source, destination, created_lt.value());
    return tonlib_component_.DoPostprocess(&core::TonlibPostProcessor::process_getTransactions, std::move(res), false, true);
  }

  if (ton_api_method == "getconfigparam") {
    auto config_id = utils::stringToInt<std::int32_t>(request.GetArg("config_id"));
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    if (!config_id.has_value()) {
      return core::TonlibWorkerResponse::from_error_string("config_id is required");
    }
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getConfigParam, config_id.value(), seqno);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }
  if (ton_api_method == "getconfigall") {
    auto seqno = utils::stringToInt<ton::BlockSeqno>(request.GetArg("seqno"));
    auto res = tonlib_component_.DoRequest(&core::TonlibWorker::getConfigAll, seqno);
    return core::TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  return {false, nullptr, std::nullopt,
    td::Status::Error(404, "method not found")};
}

}  // namespace ton_http::handlers
