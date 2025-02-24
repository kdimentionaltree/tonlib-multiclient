#pragma once
#include "tonlib-multiclient/multi_client.h"
#include "tl/tl_json.h"
#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api_json.h"
#include "td/utils/JsonBuilder.h"
#include "tonlib-multiclient/request.h"
#include "userver/engine/future.hpp"

namespace ton_http::core {
// new schemas
struct DetectAddressResult {
  block::StdAddress address;
  std::string given_type;

  [[nodiscard]] std::string to_json_string() const;
  [[nodiscard]] std::string to_raw_form(bool lower=false) const;
};

struct DetectHashResult {
  std::string hash;
  [[nodiscard]] std::string to_json_string() const;
};

struct ConsensusBlockResult {
  std::int32_t seqno;
  std::time_t timestamp;
  [[nodiscard]] std::string to_json_string() const;
};

// TonlibWorker
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
  static TonlibWorkerResponse from_result_string(const std::string& result) {
    return {true, nullptr, result, std::nullopt};
  }
  static TonlibWorkerResponse from_error_string(const std::string& error, const int code = 0) {
    return {false, nullptr, std::nullopt, td::Status::Error(code, error)};
  }
};

class TonlibWorker {
public:
  explicit TonlibWorker(const multiclient::MultiClientConfig& config) : tonlib_(config) {};
  ~TonlibWorker() = default;

  td::Result<ConsensusBlockResult> getConsensusBlock() const;
  td::Result<DetectAddressResult> detectAddress(const std::string& address) const;
  td::Result<std::string> packAddress(const std::string& address) const;
  td::Result<std::string> unpackAddress(const std::string& address) const;
  td::Result<DetectHashResult> detectHash(const std::string& hash) const;
  td::Result<tonlib_api::blocks_getMasterchainInfo::ReturnType>
    getMasterchainInfo() const;
  td::Result<tonlib_api::blocks_getMasterchainBlockSignatures::ReturnType>
    getMasterchainBlockSignatures(ton::BlockSeqno seqno) const;
  td::Result<tonlib_api::raw_getAccountState::ReturnType>
    getAddressInformation(std::string address, std::optional<std::int32_t> seqno = std::nullopt) const;
  td::Result<tonlib_api::getAccountState::ReturnType>
    getExtendedAddressInformation(std::string address, std::optional<std::int32_t> seqno = std::nullopt) const;
  td::Result<tonlib_api::blocks_lookupBlock::ReturnType> lookupBlock(const ton::WorkchainId& workchain,
    const ton::ShardId& shard, const std::optional<ton::BlockSeqno>& seqno = std::nullopt,
    const std::optional<ton::LogicalTime>& lt = std::nullopt,
    const std::optional<ton::UnixTime>& unixtime = std::nullopt) const;
  td::Result<tonlib_api::blocks_getShardBlockProof::ReturnType> getShardBlockProof(const ton::WorkchainId& workchain,
    const ton::ShardId& shard, const ton::BlockSeqno& seqno,
    const std::optional<ton::BlockSeqno>& from_seqno = std::nullopt) const;
  td::Result<tonlib_api::blocks_getShards::ReturnType> getShards(std::optional<ton::BlockSeqno> mc_seqno = std::nullopt,
    std::optional<ton::LogicalTime> lt = std::nullopt, std::optional<ton::UnixTime> unixtime = std::nullopt) const;
  td::Result<tonlib_api::blocks_getBlockHeader::ReturnType> getBlockHeader(
      const ton::WorkchainId& workchain,
      const ton::ShardId& shard,
      const ton::BlockSeqno& seqno,
      const std::string& root_hash = "",
      const std::string& file_hash = ""
  ) const;
  td::Result<tonlib_api::blocks_getOutMsgQueueSizes::ReturnType> getOutMsgQueueSizes() const;

  td::Result<tonlib_api::getConfigParam::ReturnType> getConfigParam(const std::int32_t& param, std::optional<ton::BlockSeqno> seqno = std::nullopt) const;
  td::Result<tonlib_api::getConfigParam::ReturnType> getConfigAll(std::optional<ton::BlockSeqno> seqno = std::nullopt) const;

  td::Result<tonlib_api::blocks_getTransactions::ReturnType> raw_getBlockTransactions(const tonlib_api::object_ptr<tonlib_api::ton_blockIdExt>& blk_id,
    size_t count, tonlib_api::object_ptr<tonlib_api::blocks_accountTransactionId>&& after = nullptr, std::optional<bool> archival = std::nullopt) const;
  td::Result<tonlib_api::blocks_getTransactionsExt::ReturnType> raw_getBlockTransactionsExt(const tonlib_api::object_ptr<tonlib_api::ton_blockIdExt>& blk_id,
    size_t count, tonlib_api::object_ptr<tonlib_api::blocks_accountTransactionId>&& after = nullptr, std::optional<bool> archival = std::nullopt) const;
  td::Result<tonlib_api::raw_getTransactions::ReturnType> raw_getTransactions(
    const std::string& account_address,
    const ton::LogicalTime& from_transaction_lt,
    const std::string& from_transaction_hash,
    const std::optional<bool> archival = std::nullopt) const;
  td::Result<tonlib_api::raw_getTransactionsV2::ReturnType> raw_getTransactionsV2(
    const std::string& account_address,
    const ton::LogicalTime& from_transaction_lt,
    const std::string& from_transaction_hash,
    const size_t count,
    const bool try_decode_messages,
    const std::optional<bool> archival = std::nullopt) const;

  td::Result<tonlib_api::blocks_getTransactions::ReturnType> getBlockTransactions(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const size_t count = 40,
    const std::string& root_hash = "",
    const std::string& file_hash = "",
    const std::optional<ton::LogicalTime>& after_lt = std::nullopt,
    const std::string after_hash = "",
    std::optional<bool> archival = std::nullopt
  ) const;

  td::Result<tonlib_api::blocks_getTransactionsExt::ReturnType> getBlockTransactionsExt(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const size_t count = 40,
    const std::string& root_hash = "",
    const std::string& file_hash = "",
    const std::optional<ton::LogicalTime>& after_lt = std::nullopt,
    const std::string after_hash = "",
    std::optional<bool> archival = std::nullopt
  ) const;

  td::Result<tonlib_api::raw_getTransactionsV2::ReturnType> getTransactions(
    const std::string& account_address,
    std::optional<ton::LogicalTime> from_transaction_lt,
    std::string from_transaction_hash,
    ton::LogicalTime to_transaction_lt = 0,
    size_t count = 10,
    size_t chunk_size = 30,
    bool try_decode_messages = true,
    std::optional<bool> archival = std::nullopt
  ) const;

  td::Result<tonlib_api::raw_getTransactionsV2::ReturnType> tryLocateTransactionByIncomingMessage(
    const std::string& source,
    const std::string& destination,
    ton::LogicalTime created_lt) const;
  td::Result<tonlib_api::raw_getTransactionsV2::ReturnType> tryLocateTransactionByOutgoingMessage(
      const std::string& source,
      const std::string& destination,
      ton::LogicalTime created_lt) const;

  td::Result<tonlib_api::raw_sendMessage::ReturnType> raw_sendMessage(
    const std::string& boc) const;
  td::Result<tonlib_api::raw_sendMessageReturnHash::ReturnType> raw_sendMessageReturnHash(
    const std::string& boc) const;

private:
  multiclient::MultiClient tonlib_;

  template<typename T>
  td::Result<typename T::ReturnType> send_request(multiclient::Request<T>&& request, bool retry_archival = false) const {
    auto result = tonlib_.send_request<T, userver::engine::Promise>(request);
    if (!retry_archival || result.is_ok()) {
      return std::move(result);
    }

    // retry request with archival
    request.parameters.archival = true;
    result = tonlib_.send_request<T, userver::engine::Promise>(request);
    return std::move(result);
  }

  template<typename T>
  td::Result<typename T::ReturnType> send_request_function(multiclient::RequestFunction<T>&& request, bool retry_archival = false) const {
    auto result = tonlib_.send_request_function<T, userver::engine::Promise>(request);
    if (!retry_archival || result.is_ok()) {
      return std::move(result);
    }
    auto error = result.move_as_error();
    // retry request with archival
    request.parameters.archival = true;
    result = tonlib_.send_request_function<T, userver::engine::Promise>(request);
    if (result.is_error()) {
      auto error_archival = result.move_as_error();
      LOG(WARNING) << error_archival.code() << " " << error_archival.message();
      if (error_archival.code() == -3) {
        return std::move(error);
      }
      return std::move(error_archival);
    }
    return std::move(result);
  }
};
}
