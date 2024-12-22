#pragma once
#include "tonlib-multiclient/multi_client.h"
#include "tl/tl_json.h"
#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api_json.h"
#include "td/utils/JsonBuilder.h"
#include "tonlib-multiclient/request.h"

namespace ton_http::core {
struct TonlibWorkerResponse {
  bool is_ok{false};
  tonlib_api::object_ptr<tonlib_api::Object> result{nullptr};
  std::optional<std::string> result_str{std::nullopt};
  std::optional<td::Status> error{std::nullopt};


  template<typename T>
  static TonlibWorkerResponse from_tonlib_result(td::Result<T>&& result) {
    if (result.is_error()) {
      return {false, nullptr, std::nullopt, result.move_as_error()};
    }
    if constexpr(std::is_same<T, std::string>::value) {
      return {true, nullptr, result.move_as_ok(), std::nullopt};
    } else {
      return {true, result.move_as_ok(), std::nullopt, std::nullopt};
    }
  }
  static TonlibWorkerResponse from_error_string(const std::string& error, const int code = 0) {
    return {false, nullptr, std::nullopt, td::Status::Error(code, error)};
  }
};

class TonlibWorker {
public:
  explicit TonlibWorker(const multiclient::MultiClientConfig& config) : tonlib_(config) {};
  ~TonlibWorker() = default;

  td::Result<tonlib_api::blocks_getMasterchainInfo::ReturnType> getMasterchainInfo() const;
  td::Result<tonlib_api::blocks_getMasterchainBlockSignatures::ReturnType> getMasterchainBlockSignatures(ton::BlockSeqno seqno) const;
  td::Result<tonlib_api::raw_getAccountState::ReturnType> getAddressInformation(std::string address, std::optional<std::int32_t> seqno = std::nullopt) const;
  td::Result<tonlib_api::getAccountState::ReturnType> getExtendedAddressInformation(std::string address, std::optional<std::int32_t> seqno = std::nullopt) const;
  td::Result<tonlib_api::blocks_lookupBlock::ReturnType> lookupBlock(const ton::WorkchainId& workchain, const ton::ShardId& shard, const std::optional<ton::BlockSeqno>& seqno = std::nullopt,
    const std::optional<ton::LogicalTime>& lt = std::nullopt, const std::optional<ton::UnixTime>& unixtime = std::nullopt) const;
private:
  multiclient::MultiClient tonlib_;

  template<typename T>
  td::Result<typename T::ReturnType> send_request(multiclient::Request<T>&& request, bool retry_archival = false) const {
    auto result = tonlib_.send_request(request);
    if (!retry_archival || result.is_ok()) {
      return std::move(result);
    }

    // retry request with archival
    request.parameters.archival = true;
    result = tonlib_.send_request(request);
    return std::move(result);
  }

  template<typename T>
  td::Result<typename T::ReturnType> send_request_function(multiclient::RequestFunction<T>&& request, bool retry_archival = false) const {
    auto result = tonlib_.send_request_function(request);
    if (!retry_archival || result.is_ok()) {
      return std::move(result);
    }

    // retry request with archival
    request.parameters.archival = true;
    result = tonlib_.send_request_function(request);
    return std::move(result);
  }
};
}
