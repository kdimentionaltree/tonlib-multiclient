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

struct RunGetMethodResult {
  tonlib_api::smc_runGetMethod::ReturnType result;
  tonlib_api::smc_getRawFullAccountState::ReturnType state;

  [[nodiscard]] std::string to_json_string() const;
};

struct TokenDataResult {
  explicit TokenDataResult(const std::string& address) : address_(address) {}
  virtual ~TokenDataResult() = default;
  std::string address_;
  [[nodiscard]] virtual std::string to_json_string() const;
};
using TokenDataResultPtr = std::unique_ptr<TokenDataResult>;

struct JettonMasterDataResult: public TokenDataResult {
  std::string total_supply_;
  bool mintable_;
  std::string admin_address_;
  bool jetton_content_onchain_;
  std::map<std::string, std::string> jetton_content_;
  std::string jetton_wallet_code_;
  explicit JettonMasterDataResult(const std::string& address) : TokenDataResult(address) {}
  [[nodiscard]] std::string to_json_string() const override;
};

struct JettonWalletDataResult: public TokenDataResult {
  std::string balance_;
  std::string owner_address_;
  std::string jetton_master_address_;
  std::optional<bool> mintless_is_claimed_;
  std::string jetton_wallet_code_;
  bool is_validated_;

  explicit JettonWalletDataResult(const std::string& address) : TokenDataResult(address) {}
  [[nodiscard]] std::string to_json_string() const override;
};

struct NFTCollectionDataResult : public TokenDataResult {
  std::string next_item_index_;
  std::string owner_address_;
  bool collection_content_onchain_;
  std::map<std::string, std::string> collection_content_;

  explicit NFTCollectionDataResult(const std::string& address) : TokenDataResult(address) {}
  [[nodiscard]] std::string to_json_string() const override;
};

struct NFTItemDataResult : public TokenDataResult {
  bool init_;
  std::string index_;
  std::string collection_address_;
  std::string owner_address_;
  bool content_onchain_;
  std::map<std::string, std::string> content_;
  bool is_validated_;
  // TODO: implement dns entry parsing
  explicit NFTItemDataResult(const std::string& address) : TokenDataResult(address) {}
  [[nodiscard]] std::string to_json_string() const override;
};

// TonlibWorker
struct TonlibWorkerResponse {
  bool is_ok{false};
  tonlib_api::object_ptr<tonlib_api::Object> result{nullptr};
  std::optional<std::string> result_str{std::nullopt};
  std::optional<td::Status> error{std::nullopt};
  multiclient::SessionPtr session{nullptr};

  template<typename T>
  static TonlibWorkerResponse from_tonlib_result(td::Result<T>&& result, multiclient::SessionPtr&& session = nullptr) {
    if (result.is_error()) {
      return {false, nullptr, std::nullopt, result.move_as_error(), std::move(session)};
    }
    if constexpr(std::is_same<T, std::string>::value) {
      return {true, nullptr, result.move_as_ok(), std::nullopt, std::move(session)};
    } else {
      return {true, result.move_as_ok(), std::nullopt, std::nullopt, std::move(session)};
    }
  }
  static TonlibWorkerResponse from_result_string(const std::string& result, multiclient::SessionPtr&& session = nullptr) {
    return {true, nullptr, result, std::nullopt, std::move(session)};
  }
  static TonlibWorkerResponse from_error_string(const std::string& error, const int code = 0, multiclient::SessionPtr&& session = nullptr) {
    return {false, nullptr, std::nullopt, td::Status::Error(code, error), std::move(session)};
  }
};

class TonlibWorker {
public:
  explicit TonlibWorker(const multiclient::MultiClientConfig& config) : tonlib_(config) {};
  ~TonlibWorker() = default;

  template<typename T>
  using Result = std::pair<td::Result<T>, multiclient::SessionPtr>;

  Result<ConsensusBlockResult> getConsensusBlock(multiclient::SessionPtr session = nullptr) const;
  Result<DetectAddressResult> detectAddress(const std::string& address, multiclient::SessionPtr session = nullptr) const;
  Result<std::string> packAddress(const std::string& address, multiclient::SessionPtr session = nullptr) const;
  Result<std::string> unpackAddress(const std::string& address, multiclient::SessionPtr session = nullptr) const;
  Result<DetectHashResult> detectHash(const std::string& hash, multiclient::SessionPtr session = nullptr) const;
  Result<TokenDataResultPtr> getTokenData(
    const std::string& address,
    bool skip_verification = false,
    std::optional<ton::BlockSeqno> seqno = std::nullopt,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;
  Result<tonlib_api::blocks_getMasterchainInfo::ReturnType>
    getMasterchainInfo(multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::blocks_getMasterchainBlockSignatures::ReturnType>
    getMasterchainBlockSignatures(ton::BlockSeqno seqno, multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::raw_getAccountState::ReturnType>
    getAddressInformation(std::string address, std::optional<std::int32_t> seqno = std::nullopt, multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::getAccountState::ReturnType>
    getExtendedAddressInformation(std::string address, std::optional<std::int32_t> seqno = std::nullopt, multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::blocks_lookupBlock::ReturnType> lookupBlock(const ton::WorkchainId& workchain,
    const ton::ShardId& shard, const std::optional<ton::BlockSeqno>& seqno = std::nullopt,
    const std::optional<ton::LogicalTime>& lt = std::nullopt,
    const std::optional<ton::UnixTime>& unixtime = std::nullopt,
    multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::blocks_getShardBlockProof::ReturnType> getShardBlockProof(const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const std::optional<ton::BlockSeqno>& from_seqno = std::nullopt,
    multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::blocks_getShards::ReturnType> getShards(std::optional<ton::BlockSeqno> mc_seqno = std::nullopt,
    std::optional<ton::LogicalTime> lt = std::nullopt,
    std::optional<ton::UnixTime> unixtime = std::nullopt,
    multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::blocks_getBlockHeader::ReturnType> getBlockHeader(
      const ton::WorkchainId& workchain,
      const ton::ShardId& shard,
      const ton::BlockSeqno& seqno,
      const std::string& root_hash = "",
      const std::string& file_hash = "",
      multiclient::SessionPtr session = nullptr
  ) const;
  Result<tonlib_api::blocks_getOutMsgQueueSizes::ReturnType> getOutMsgQueueSizes(multiclient::SessionPtr session = nullptr) const;

  Result<tonlib_api::getConfigParam::ReturnType> getConfigParam(const std::int32_t& param,
    std::optional<ton::BlockSeqno> seqno = std::nullopt,
    multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::getConfigParam::ReturnType> getConfigAll(std::optional<ton::BlockSeqno> seqno = std::nullopt, multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::smc_getLibraries::ReturnType> getLibraries(
    std::vector<std::string> libs,
    multiclient::SessionPtr session = nullptr) const;

  Result<tonlib_api::blocks_getTransactions::ReturnType> raw_getBlockTransactions(const tonlib_api::object_ptr<tonlib_api::ton_blockIdExt>& blk_id,
    size_t count,
    tonlib_api::object_ptr<tonlib_api::blocks_accountTransactionId>&& after = nullptr,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::blocks_getTransactionsExt::ReturnType> raw_getBlockTransactionsExt(const tonlib_api::object_ptr<tonlib_api::ton_blockIdExt>& blk_id,
    size_t count,
    tonlib_api::object_ptr<tonlib_api::blocks_accountTransactionId>&& after = nullptr,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::raw_getTransactions::ReturnType> raw_getTransactions(
    const std::string& account_address,
    const ton::LogicalTime& from_transaction_lt,
    const std::string& from_transaction_hash,
    const std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::raw_getTransactionsV2::ReturnType> raw_getTransactionsV2(
    const std::string& account_address,
    const ton::LogicalTime& from_transaction_lt,
    const std::string& from_transaction_hash,
    const size_t count,
    const bool try_decode_messages,
    const std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr) const;

  Result<tonlib_api::blocks_getTransactions::ReturnType> getBlockTransactions(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const size_t count = 40,
    const std::string& root_hash = "",
    const std::string& file_hash = "",
    const std::optional<ton::LogicalTime>& after_lt = std::nullopt,
    const std::string& after_hash = "",
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;

  Result<tonlib_api::blocks_getTransactionsExt::ReturnType> getBlockTransactionsExt(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const size_t count = 40,
    const std::string& root_hash = "",
    const std::string& file_hash = "",
    const std::optional<ton::LogicalTime>& after_lt = std::nullopt,
    const std::string& after_hash = "",
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;

  Result<tonlib_api::raw_getTransactionsV2::ReturnType> getTransactions(
    const std::string& account_address,
    std::optional<ton::LogicalTime> from_transaction_lt,
    std::string from_transaction_hash,
    ton::LogicalTime to_transaction_lt = 0,
    size_t count = 10,
    size_t chunk_size = 30,
    bool try_decode_messages = true,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;

  Result<tonlib_api::raw_getTransactionsV2::ReturnType> tryLocateTransactionByIncomingMessage(
    const std::string& source,
    const std::string& destination,
    ton::LogicalTime created_lt,
    multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::raw_getTransactionsV2::ReturnType> tryLocateTransactionByOutgoingMessage(
      const std::string& source,
      const std::string& destination,
      ton::LogicalTime created_lt,
      multiclient::SessionPtr session = nullptr) const;

  Result<tonlib_api::raw_sendMessage::ReturnType> raw_sendMessage(
    const std::string& boc,
    multiclient::SessionPtr session = nullptr) const;
  Result<tonlib_api::raw_sendMessageReturnHash::ReturnType> raw_sendMessageReturnHash(
    const std::string& boc,
    multiclient::SessionPtr session = nullptr) const;

  Result<tonlib_api::smc_load::ReturnType> loadContract(
    const std::string& address,
    std::optional<ton::BlockSeqno> seqno = std::nullopt,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;

  Result<tonlib_api::smc_forget::ReturnType> forgetContract(
    std::int64_t id,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;

  Result<RunGetMethodResult> runGetMethod(
    const std::string& address,
    const std::string& method_name,
    const std::vector<std::string>& stack,
    std::optional<ton::BlockSeqno> seqno = std::nullopt,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;

  Result<tonlib_api::query_estimateFees::ReturnType> queryEstimateFees(
    const std::string& account_address,
    const std::string& body,
    const std::string& init_code = "",
    const std::string& init_data = "",
    bool ignore_chksig = true,
    multiclient::SessionPtr session = nullptr
  ) const;

private:
  multiclient::MultiClient tonlib_;

  Result<TokenDataResultPtr> checkJettonMaster(
    const std::string& address,
    bool skip_verification = false,
    std::optional<ton::BlockSeqno> seqno = std::nullopt,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;
  Result<TokenDataResultPtr> checkJettonWallet(
    const std::string& address,
    bool skip_verification = false,
    std::optional<ton::BlockSeqno> seqno = std::nullopt,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;
  Result<TokenDataResultPtr> checkNFTCollection(
    const std::string& address,
    bool skip_verification = false,
    std::optional<ton::BlockSeqno> seqno = std::nullopt,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;
  Result<TokenDataResultPtr> checkNFTItem(
    const std::string& address,
    bool skip_verification = false,
    std::optional<ton::BlockSeqno> seqno = std::nullopt,
    std::optional<bool> archival = std::nullopt,
    multiclient::SessionPtr session = nullptr
  ) const;

  template<typename T>
  Result<typename T::ReturnType> send_request_function(multiclient::RequestFunction<T>&& request, bool retry_archival = false) const {
    if (!request.session) {
      auto r_session = tonlib_.get_session(request.parameters, std::move(request.session));
      if (r_session.is_error()) {
        return std::make_pair(r_session.move_as_error(), request.session);
      }
      request.session = r_session.move_as_ok();
    }

    auto result = tonlib_.send_request_function<T, userver::engine::Promise>(request);
    if (!retry_archival || result.is_ok()) {
      return std::make_pair(std::move(result), request.session);
    }
    auto error = result.move_as_error();

    // retry request with archival
    {
      request.parameters.archival = true;
      auto r_session = tonlib_.get_session(request.parameters, std::move(request.session));
      if (r_session.is_error()) {
        return std::make_pair(r_session.move_as_error(), request.session);
      }
      request.session = r_session.move_as_ok();
    }

    result = tonlib_.send_request_function<T, userver::engine::Promise>(request);
    if (result.is_error()) {
      auto error_archival = result.move_as_error();
      LOG(WARNING) << error_archival.code() << " " << error_archival.message();
      if (error_archival.code() == -3) {
        return std::make_pair(std::move(error), request.session);
      }
      return std::make_pair(std::move(error_archival), request.session);
    }
    return std::make_pair(std::move(result), request.session);
  }
};
}
