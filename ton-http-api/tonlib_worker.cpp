#include "tonlib_worker.h"

#include "userver/formats/json.hpp"
#include "utils.hpp"
#include "tvm_utils.h"

namespace ton_http::core {

template<typename T>
auto tl_to_json(const T& value) {
  return userver::formats::json::FromString(td::json_encode<td::string>(td::ToJson(value)));
}

template <typename T1, typename T2>
auto value_or_default(const std::optional<T1>& arg, const T2& def) {
  return (arg.has_value() ? arg.value() : (def));
}

std::string DetectAddressResult::to_json_string() const {
  using namespace userver::formats::json;

  ValueBuilder builder;
  builder["raw_form"] = to_raw_form(true);
  for (const bool bounce : {true, false}) {
    block::StdAddress b_addr(address);
    b_addr.bounceable = bounce;
    const std::string field_name = (bounce ? "bounceable" : "non_bounceable");
    builder[field_name]["b64"] = b_addr.rserialize(false);
    builder[field_name]["b64url"] = b_addr.rserialize(true);
  }
  builder["given_type"] = given_type;
  builder["test_only"] = address.testnet;

  auto res = ToString(builder.ExtractValue());
  return std::move(res);
}
std::string DetectAddressResult::to_raw_form(bool lower) const {
  td::StringBuilder sb;
  sb << address.workchain << ":" << address.addr.to_hex();
  auto raw_form = sb.as_cslice().str();
  if (lower) {
    std::ranges::transform(raw_form, raw_form.begin(), ::tolower);
  }
  return std::move(raw_form);
}
std::string DetectHashResult::to_json_string() const {
  using namespace userver::formats::json;

  ValueBuilder builder;
  builder["b64"] = td::base64_encode(hash);
  builder["b64url"] = td::base64url_encode(hash);
  builder["hex"] = td::hex_encode(hash);
  auto res = ToString(builder.ExtractValue());
  return std::move(res);
}
std::string ConsensusBlockResult::to_json_string() const {
  using namespace userver::formats::json;

  ValueBuilder builder;
  builder["consensus_block"] = seqno;
  builder["timestamp"] = timestamp;
  auto res = ToString(builder.ExtractValue());
  return std::move(res);
}
std::string RunGetMethodResult::to_json_string() const {
  using namespace userver::formats::json;

  ValueBuilder builder(tl_to_json(result));
  builder["block_id"] = tl_to_json(state->block_id_);
  builder["last_transaction_id"] = tl_to_json(state->last_transaction_id_);
  return std::move(ToString(builder.ExtractValue()));
}

std::string TokenDataResult::to_json_string() const {
  return "{\"error\": \"why?\"}";
}

std::string JettonMasterDataResult::to_json_string() const {
  using namespace userver::formats::json;

  ValueBuilder builder;
  builder["address"] = address_;
  builder["contract_type"] = "jetton_master";
  builder["total_supply"] = total_supply_;
  builder["mintable"] = mintable_;
  builder["admin_address"] = admin_address_;
  if (jetton_content_onchain_) {
    builder["jetton_content"]["type"] = "onchain";
    for (auto& [k, v] : jetton_content_) {
      builder["jetton_content"]["data"][k] = v;
    }
  } else {
    builder["jetton_content"]["type"] = "offchain";
    if (jetton_content_.contains("uri")) {
      builder["jetton_content"]["data"] = jetton_content_.at("uri");
    }
  }
  builder["jetton_wallet_code"] = jetton_wallet_code_;
  return std::move(ToString(builder.ExtractValue()));
}

std::string JettonWalletDataResult::to_json_string() const {
  using namespace userver::formats::json;

  ValueBuilder builder;
  builder["address"] = address_;
  builder["contract_type"] = "jetton_wallet";
  builder["balance"] = balance_;
  builder["owner"] = owner_address_;
  builder["jetton"] = jetton_master_address_;
  builder["jetton_wallet_code"] = jetton_wallet_code_;
  if (mintless_is_claimed_.has_value()) {
    builder["mintless_is_claimed"] = mintless_is_claimed_.value();
  }
  builder["is_validated"] = is_validated_;
  return std::move(ToString(builder.ExtractValue()));
}

std::string NFTCollectionDataResult::to_json_string() const {
  using namespace userver::formats::json;

  ValueBuilder builder;
  builder["address"] = address_;
  builder["contract_type"] = "nft_collection";
  builder["next_item_index"] = next_item_index_;
  if (owner_address_.length() > 0) {
    builder["owner_address"] = owner_address_;
  }
  if (collection_content_onchain_) {
    builder["collection_content"]["type"] = "onchain";
    for (auto& [k, v] : collection_content_) {
      builder["collection_content"]["data"][k] = v;
    }
  } else {
    builder["collection_content"]["type"] = "offchain";
    if (collection_content_.contains("uri")) {
      builder["collection_content"]["data"] = collection_content_.at("uri");
    }
  }
  return std::move(ToString(builder.ExtractValue()));
}

std::string NFTItemDataResult::to_json_string() const {
  using namespace userver::formats::json;

  ValueBuilder builder;
  builder["address"] = address_;
  builder["contract_type"] = "nft_item";
  builder["init"] = init_;
  builder["index"] = index_;
  if (!owner_address_.empty()) {
    builder["owner_address"] = owner_address_;
  }
  if (!collection_address_.empty()) {
    builder["collection_address"] = collection_address_;
  }
  if (content_onchain_) {
    builder["content"]["type"] = "onchain";
    for (auto& [k, v] : content_) {
      builder["content"]["data"][k] = v;
    }
  } else {
    builder["content"]["type"] = "offchain";
    if (content_.contains("uri")) {
      builder["content"]["data"] = content_.at("uri");
    }
  }
  builder["is_validated"] = is_validated_;
  return std::move(ToString(builder.ExtractValue()));
}

TonlibWorker::Result<ConsensusBlockResult> TonlibWorker::getConsensusBlock(multiclient::SessionPtr session) const {
  auto res = tonlib_.get_consensus_block();
  if (res.is_error()) {
    return {res.move_as_error(), session};
  }
  return {ConsensusBlockResult{ res.move_as_ok(), std::time(nullptr) }, session};
}
TonlibWorker::Result<DetectAddressResult> TonlibWorker::detectAddress(const std::string& address, multiclient::SessionPtr session) const {
  auto r_std_address = block::StdAddress::parse(address);
  if (r_std_address.is_error()) {
    return {r_std_address.move_as_error(), session};
  }
  const auto std_address = r_std_address.move_as_ok();
  std::string given_type = "raw_form";
  if (address.length() == 48) {
    given_type = std::string("friendly_") + (std_address.bounceable ? "bounceable" : "non_bounceable");
  }
  DetectAddressResult result{std_address, given_type};
  return {std::move(result), session};
}
TonlibWorker::Result<std::string> TonlibWorker::packAddress(const std::string& address, multiclient::SessionPtr session) const {
  auto r_std_address = block::StdAddress::parse(address);
  if (r_std_address.is_error()) {
    return {r_std_address.move_as_error(), session};
  }
  const auto std_address = r_std_address.move_as_ok();
  return {std::move(std_address.rserialize(true)), session};
}

TonlibWorker::Result<std::string> TonlibWorker::unpackAddress(const std::string& address, multiclient::SessionPtr session) const {
  block::StdAddress std_address;
  if (std_address.rdeserialize(address)) {
    const DetectAddressResult result{std_address, "unknown"};
    return {result.to_raw_form(true), session};
  }
}
TonlibWorker::Result<DetectHashResult> TonlibWorker::detectHash(
    const std::string& hash, multiclient::SessionPtr session
) const {
  auto raw_hash = utils::stringToHash(hash);
  if (!raw_hash.has_value()) {
    return {td::Status::Error("Invalid hash"), session};
  }
  DetectHashResult result{raw_hash.value()};
  return {std::move(result), session};
}

TonlibWorker::Result<TokenDataResultPtr> TonlibWorker::getTokenData(
  const std::string& address,
  bool skip_verification,
  std::optional<ton::BlockSeqno> seqno,
  std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  auto [r_jetton_master, _s1] = checkJettonMaster(address, skip_verification, seqno, archival, session);
  auto [r_jetton_wallet, _s2] = checkJettonWallet(address, skip_verification, seqno, archival, session);
  auto [r_nft_collection, _s3] = checkNFTCollection(address, skip_verification, seqno, archival, session);
  auto [r_nft_item, _s4] = checkNFTItem(address, skip_verification, seqno, archival, session);

  if (r_jetton_master.is_ok()) {
    auto data = r_jetton_master.move_as_ok();
    if (data) {
      LOG(ERROR) << "1";
      return {std::move(data), std::move(session)};
    }
  } else {
    LOG(ERROR) << "Jetton master: " << r_jetton_master.move_as_error();
  }
  if (r_jetton_wallet.is_ok()) {
    auto data = r_jetton_wallet.move_as_ok();
    if (data) {
      LOG(ERROR) << "2";
      return {std::move(data), std::move(session)};
    }
  } else {
    LOG(ERROR) << "Jetton wallet: " << r_jetton_wallet.move_as_error();
  }

  if (r_nft_collection.is_ok()) {
    auto data = r_nft_collection.move_as_ok();
    if (data) {
      LOG(ERROR) << "3";
      return {std::move(data), std::move(session)};
    }
  } else {
    LOG(ERROR) << "NFT collection: " << r_nft_collection.move_as_error();
  }

  if (r_nft_item.is_ok()) {
    auto data = r_nft_item.move_as_ok();
    if (data) {
      LOG(ERROR) << "4";
      return {std::move(data), std::move(session)};
    }
  } else {
    LOG(ERROR) << "NFT item: " << r_nft_item.move_as_error();
  }

  LOG(ERROR) << "5";
  return {td::Status::Error(409, "Smart contract is not Jetton or NFT"), std::move(session)};
}
TonlibWorker::Result<tonlib_api::blocks_getMasterchainInfo::ReturnType> TonlibWorker::getMasterchainInfo(multiclient::SessionPtr session) const {
  auto request = multiclient::RequestFunction<tonlib_api::blocks_getMasterchainInfo>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator = [] { return tonlib_api::make_object<tonlib_api::blocks_getMasterchainInfo>(); },
      .session = std::move(session)
  };
  auto [result, new_session] = send_request_function(std::move(request), true);
  return {std::move(result), new_session};
}
TonlibWorker::Result<tonlib_api::blocks_getMasterchainBlockSignatures::ReturnType> TonlibWorker::getMasterchainBlockSignatures(ton::BlockSeqno seqno,
    multiclient::SessionPtr session) const {
  auto request = multiclient::RequestFunction<tonlib_api::blocks_getMasterchainBlockSignatures>{
    .parameters = {.mode = multiclient::RequestMode::Single},
    .request_creator = [seqno] { return tonlib_api::make_object<tonlib_api::blocks_getMasterchainBlockSignatures>(seqno); },
    .session = std::move(session)
  };
  auto [result, new_session] = send_request_function(std::move(request), true);
  return {std::move(result), session};
}
TonlibWorker::Result<tonlib_api::raw_getAccountState::ReturnType> TonlibWorker::getAddressInformation(
    std::string address,
    std::optional<std::int32_t> seqno,
    multiclient::SessionPtr session
) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> with_block;
  if (seqno.has_value()) {
    auto [res, new_session] = lookupBlock(ton::masterchainId,
      ton::shardIdAll, seqno.value(), std::nullopt, std::nullopt, session);
    if (!res.is_ok()) {
      return {res.move_as_error(), new_session};
    }
    with_block = res.move_as_ok();
    session = std::move(new_session);
  }
  if (!with_block) {
    auto request = multiclient::RequestFunction<tonlib_api::raw_getAccountState>{
        .parameters = {.mode = multiclient::RequestMode::Single},
        .request_creator =
            [address_ = address] {
              return tonlib_api::make_object<tonlib_api::raw_getAccountState>(
                  tonlib_api::make_object<tonlib_api::accountAddress>(address_)
              );
            },
        .session = session
    };
    auto [result, new_sesion] = send_request_function(std::move(request), true);
    return {std::move(result), new_sesion};
  } else {
    auto request = multiclient::RequestFunction<tonlib_api::withBlock>{
        .parameters = {.mode = multiclient::RequestMode::Single},
        .request_creator =
            [address_ = address,
             workchain_ = with_block->workchain_,
             shard_ = with_block->shard_,
             seqno_ = with_block->seqno_,
             root_hash_ = with_block->root_hash_,
             file_hash_ = with_block->file_hash_] {
              return tonlib_api::make_object<tonlib_api::withBlock>(
                  tonlib_api::make_object<tonlib_api::ton_blockIdExt>(
                      workchain_, shard_, seqno_, root_hash_, file_hash_
                  ),
                  tonlib_api::make_object<tonlib_api::raw_getAccountState>(
                      tonlib_api::make_object<tonlib_api::accountAddress>(address_)
                  )
              );
            },
        .session = session
    };
    auto [result, new_session] = send_request_function(std::move(request), true);
    if (result.is_error()) {
      return {result.move_as_error(), new_session};
    }
    return {ton::move_tl_object_as<tonlib_api::raw_fullAccountState>(result.move_as_ok()), new_session};
  }
}
TonlibWorker::Result<tonlib_api::getAccountState::ReturnType> TonlibWorker::getExtendedAddressInformation(
    std::string address,
    std::optional<std::int32_t> seqno,
    multiclient::SessionPtr session
) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> with_block;
  if (seqno.has_value()) {
    auto [res, new_session] = lookupBlock(ton::masterchainId, ton::shardIdAll,
      seqno.value(), std::nullopt, std::nullopt, session);
    if (!res.is_ok()) {
      return {res.move_as_error(), new_session};
    }
    with_block = res.move_as_ok();
    session = std::move(new_session);
  }
  if (!with_block) {
    auto request = multiclient::RequestFunction<tonlib_api::getAccountState>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [address_ = address] {
            return tonlib_api::make_object<tonlib_api::getAccountState>(
                tonlib_api::make_object<tonlib_api::accountAddress>(address_)
            );
      },
      .session = session
    };
    auto [result, new_session] = send_request_function(std::move(request), true);
    return {std::move(result), new_session};
  }
  auto request = multiclient::RequestFunction<tonlib_api::withBlock>{
    .parameters = {.mode = multiclient::RequestMode::Single},
    .request_creator =
        [address_ = address,
         workchain_ = with_block->workchain_,
         shard_ = with_block->shard_,
         seqno_ = with_block->seqno_,
         root_hash_ = with_block->root_hash_,
         file_hash_ = with_block->file_hash_] {
          return tonlib_api::make_object<tonlib_api::withBlock>(
              tonlib_api::make_object<tonlib_api::ton_blockIdExt>(
                  workchain_, shard_, seqno_, root_hash_, file_hash_
              ),
              tonlib_api::make_object<tonlib_api::getAccountState>(
                  tonlib_api::make_object<tonlib_api::accountAddress>(address_)
              )
          );
    },
    .session = session
  };
  auto [result, new_session] = send_request_function(std::move(request), true);
  if (result.is_error()) {
    return {result.move_as_error(), new_session};
  }
  return {ton::move_tl_object_as<tonlib_api::fullAccountState>(result.move_as_ok()), session};
}
TonlibWorker::Result<tonlib_api::blocks_lookupBlock::ReturnType> TonlibWorker::lookupBlock(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const std::optional<ton::BlockSeqno>& seqno,
    const std::optional<ton::LogicalTime>& lt,
    const std::optional<ton::UnixTime>& unixtime,
    multiclient::SessionPtr session
) const {
  if (!(seqno.has_value() || lt.has_value() || unixtime.has_value())) {
    return {td::Status::Error(416, "one of seqno, lt, unixtime should be specified"), session};
  }
  int lookupMode = 0;
  if (seqno.has_value()) {
    lookupMode += 1;
  }
  if (lt.has_value()) {
    lookupMode += 2;
  }
  if (unixtime.has_value()) {
    lookupMode += 4;
  }

  // try non-archival
  auto request = multiclient::RequestFunction<tonlib_api::blocks_lookupBlock>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [lookupMode, workchain, shard, seqno, lt, unixtime] {
            return tonlib_api::make_object<tonlib_api::blocks_lookupBlock>(
                lookupMode,
                tonlib_api::make_object<tonlib_api::ton_blockId>(workchain, shard, value_or_default(seqno, 0)),
                value_or_default(lt, 0),
                value_or_default(unixtime, 0)
            );
          },
      .session = session
  };
  auto [result, new_session] = send_request_function(std::move(request), true);
  return {std::move(result), new_session};
}
TonlibWorker::Result<tonlib_api::blocks_getShardBlockProof::ReturnType> TonlibWorker::getShardBlockProof(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const std::optional<ton::BlockSeqno>& from_seqno,
    multiclient::SessionPtr session
) const {
  std::int32_t mode = 0;
  auto [r_blk_id, new_session] = lookupBlock(workchain, shard, seqno,
    std::nullopt, std::nullopt, session);
  if (r_blk_id.is_error()) {
    return {r_blk_id.move_as_error(), new_session};
  }
  auto blk_id = r_blk_id.move_as_ok();
  session = std::move(new_session);

  auto request = multiclient::RequestFunction<tonlib_api::blocks_getShardBlockProof>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [mode,
           w = blk_id->workchain_,
           s = blk_id->shard_,
           ss = blk_id->seqno_,
           r = blk_id->root_hash_,
           f = blk_id->file_hash_] {
            return tonlib_api::make_object<tonlib_api::blocks_getShardBlockProof>(
                tonlib_api::make_object<tonlib_api::ton_blockIdExt>(w, s, ss, r, f), mode, nullptr
            );
          },
      .session = session
  };

  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> from_blk_id = nullptr;
  if (from_seqno.has_value()) {
    mode = 1;
    auto [r_from_blk_id, new_session] = lookupBlock(ton::masterchainId, ton::shardIdAll, from_seqno.value(), std::nullopt, std::nullopt, session);
    if (r_from_blk_id.is_error()) {
      return {r_from_blk_id.move_as_error_prefix("lookup from block error: "), new_session};
    }
    from_blk_id = r_from_blk_id.move_as_ok();
    session = new_session;
    request.request_creator = [mode,
                               w = blk_id->workchain_,
                               s = blk_id->shard_,
                               ss = blk_id->seqno_,
                               r = blk_id->root_hash_,
                               f = blk_id->file_hash_,
                               fw = from_blk_id->workchain_,
                               fs = from_blk_id->shard_,
                               fss = from_blk_id->seqno_,
                               fr = from_blk_id->root_hash_,
                               ff = from_blk_id->file_hash_] {
      return tonlib_api::make_object<tonlib_api::blocks_getShardBlockProof>(
          tonlib_api::make_object<tonlib_api::ton_blockIdExt>(w, s, ss, r, f),
          mode,
          tonlib_api::make_object<tonlib_api::ton_blockIdExt>(fw, fs, fss, fr, ff)
      );
    };
  }
  auto [result, new_session_2] = send_request_function(std::move(request), true);
  return {std::move(result), new_session_2};
}
TonlibWorker::Result<tonlib_api::blocks_getShards::ReturnType> TonlibWorker::getShards(
    std::optional<ton::BlockSeqno> mc_seqno,
    std::optional<ton::LogicalTime> lt,
    std::optional<ton::UnixTime> unixtime,
    multiclient::SessionPtr session
) const {
  auto [r_blk_id, new_session] = lookupBlock(ton::masterchainId, ton::shardIdAll, mc_seqno, lt, unixtime, session);
  if (r_blk_id.is_error()) {
    return {r_blk_id.move_as_error(), new_session};
  }
  auto blk_id = r_blk_id.move_as_ok();
  auto request = multiclient::RequestFunction<tonlib_api::blocks_getShards>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [w = blk_id->workchain_,
           s = blk_id->shard_,
           ss = blk_id->seqno_,
           r = blk_id->root_hash_,
           f = blk_id->file_hash_] {
            return tonlib_api::make_object<tonlib_api::blocks_getShards>(
                tonlib_api::make_object<tonlib_api::ton_blockIdExt>(w, s, ss, r, f)
            );
          },
      .session = session
  };
  auto [result, new_session_2] = send_request_function(std::move(request), true);
  return {std::move(result), new_session_2};
}
TonlibWorker::Result<tonlib_api::blocks_getBlockHeader::ReturnType> TonlibWorker::getBlockHeader(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const std::string& root_hash,
    const std::string& file_hash,
    multiclient::SessionPtr session
) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> blk_id = nullptr;
  if (!root_hash.empty() && !file_hash.empty()) {
    blk_id = tonlib_api::make_object<tonlib_api::ton_blockIdExt>(workchain, shard, seqno, root_hash, file_hash);
  } else {
    auto [r_blk_id, new_session] = lookupBlock(workchain, shard, seqno, std::nullopt, std::nullopt, session);
    if (r_blk_id.is_error()) {
      return {r_blk_id.move_as_error(), new_session};
    }
    blk_id = r_blk_id.move_as_ok();
    session = std::move(new_session);
  }
  auto request = multiclient::RequestFunction<tonlib_api::blocks_getBlockHeader>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [w = blk_id->workchain_,
           s = blk_id->shard_,
           ss = blk_id->seqno_,
           r = blk_id->root_hash_,
           f = blk_id->file_hash_] {
            return tonlib_api::make_object<tonlib_api::blocks_getBlockHeader>(
                tonlib_api::make_object<tonlib_api::ton_blockIdExt>(w, s, ss, r, f)
            );
          },
      .session = std::move(session)
  };
  auto [result, new_session] = send_request_function(std::move(request), true);
  return {std::move(result), new_session};
}
TonlibWorker::Result<tonlib_api::blocks_getOutMsgQueueSizes::ReturnType> TonlibWorker::getOutMsgQueueSizes(multiclient::SessionPtr session) const {
  auto request = multiclient::RequestFunction<tonlib_api::blocks_getOutMsgQueueSizes>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [] {
            return tonlib_api::make_object<tonlib_api::blocks_getOutMsgQueueSizes>();
          },
      .session = std::move(session)
  };
  auto [result, new_session] = send_request_function(std::move(request), true);
  return {std::move(result), new_session};
}
TonlibWorker::Result<tonlib_api::getConfigParam::ReturnType> TonlibWorker::getConfigParam(
    const std::int32_t& param,
    std::optional<ton::BlockSeqno> seqno,
    multiclient::SessionPtr session
) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> with_block;
  if (seqno.has_value()) {
    auto [res, new_session] = lookupBlock(ton::masterchainId, ton::shardIdAll, seqno.value());
    if (!res.is_ok()) {
      return {res.move_as_error(), new_session};
    }
    with_block = res.move_as_ok();
    session = std::move(new_session);
  }
  if (!with_block) {
    auto request = multiclient::RequestFunction<tonlib_api::getConfigParam>{
        .parameters = {.mode = multiclient::RequestMode::Single},
        .request_creator =
            [param_ = param] {
              return tonlib_api::make_object<tonlib_api::getConfigParam>(param_, 0);
            },
        .session = session
    };
    auto [result, new_session] = send_request_function(std::move(request), true);
    return {std::move(result), new_session};
  } else {
    auto request = multiclient::RequestFunction<tonlib_api::withBlock>{
        .parameters = {.mode = multiclient::RequestMode::Single},
        .request_creator =
            [param_ = param,
             workchain_ = with_block->workchain_,
             shard_ = with_block->shard_,
             seqno_ = with_block->seqno_,
             root_hash_ = with_block->root_hash_,
             file_hash_ = with_block->file_hash_] {
              return tonlib_api::make_object<tonlib_api::withBlock>(
                  tonlib_api::make_object<tonlib_api::ton_blockIdExt>(
                      workchain_, shard_, seqno_, root_hash_, file_hash_
                  ),
                  tonlib_api::make_object<tonlib_api::getConfigParam>(param_, 0)
              );
            },
        .session = session
    };
    auto [result, new_session] = send_request_function(std::move(request), true);
    if (result.is_error()) {
      return {result.move_as_error(), new_session};
    }
    return {ton::move_tl_object_as<tonlib_api::configInfo>(result.move_as_ok()), new_session};
  }
}
TonlibWorker::Result<tonlib_api::getConfigParam::ReturnType> TonlibWorker::getConfigAll(std::optional<ton::BlockSeqno> seqno, multiclient::SessionPtr session) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> with_block;
  if (seqno.has_value()) {
    auto [res, new_session] = lookupBlock(ton::masterchainId, ton::shardIdAll, seqno.value(), std::nullopt, std::nullopt, session);
    if (!res.is_ok()) {
      return {res.move_as_error(), new_session};
    }
    with_block = res.move_as_ok();
    session = std::move(new_session);
  }
  if (!with_block) {
    auto request = multiclient::RequestFunction<tonlib_api::getConfigAll>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [] {
            return tonlib_api::make_object<tonlib_api::getConfigAll>(0);
      },
      .session = std::move(session)
    };
    auto [result, new_session] = send_request_function(std::move(request), true);
    return {std::move(result), new_session};
  } else {
    auto request = multiclient::RequestFunction<tonlib_api::withBlock>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [workchain_ = with_block->workchain_,
          shard_ = with_block->shard_,
          seqno_ = with_block->seqno_,
          root_hash_ = with_block->root_hash_,
          file_hash_ = with_block->file_hash_] {
            return tonlib_api::make_object<tonlib_api::withBlock>(
                tonlib_api::make_object<tonlib_api::ton_blockIdExt>(
                    workchain_, shard_, seqno_, root_hash_, file_hash_
                ),
                tonlib_api::make_object<tonlib_api::getConfigAll>(0)
            );
      },
      .session = std::move(session)
    };
    auto [result, new_session] = send_request_function(std::move(request), true);
    session = std::move(new_session);
    if (result.is_error()) {
      return {result.move_as_error(), session};
    }
    return {ton::move_tl_object_as<tonlib_api::configInfo>(result.move_as_ok()), session};
  }
}
TonlibWorker::Result<tonlib_api::blocks_getTransactions::ReturnType> TonlibWorker::raw_getBlockTransactions(
    const tonlib_api::object_ptr<tonlib_api::ton_blockIdExt>& blk_id,
    size_t count,
    tonlib_api::object_ptr<tonlib_api::blocks_accountTransactionId>&& after,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  auto request = multiclient::RequestFunction<tonlib_api::blocks_getTransactions>{
      .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
      .request_creator =
          [w = blk_id->workchain_,
           sh = blk_id->shard_,
           s = blk_id->seqno_,
           r = blk_id->root_hash_,
           f = blk_id->file_hash_,
           c = count] {
            std::int32_t mode = 7;
            return tonlib_api::make_object<tonlib_api::blocks_getTransactions>(
                tonlib_api::make_object<tonlib_api::ton_blockIdExt>(w, sh, s, r, f), mode, c, nullptr
            );
          },
      .session = std::move(session)
  };
  if (after) {
    request.request_creator = [w = blk_id->workchain_,
                               sh = blk_id->shard_,
                               s = blk_id->seqno_,
                               r = blk_id->root_hash_,
                               f = blk_id->file_hash_,
                               a = after->account_,
                               l = after->lt_,
                               c = count] {
      std::int32_t mode = 7 + 128;
      return tonlib_api::make_object<tonlib_api::blocks_getTransactions>(
          tonlib_api::make_object<tonlib_api::ton_blockIdExt>(w, sh, s, r, f),
          mode,
          c,
          tonlib_api::make_object<tonlib_api::blocks_accountTransactionId>(a, l)
      );
    };
  }
  auto [result, new_session] = send_request_function(std::move(request), !archival.has_value());
  return {std::move(result), new_session};
}
TonlibWorker::Result<tonlib_api::blocks_getTransactionsExt::ReturnType> TonlibWorker::raw_getBlockTransactionsExt(
    const tonlib_api::object_ptr<tonlib_api::ton_blockIdExt>& blk_id,
    size_t count,
    tonlib_api::object_ptr<tonlib_api::blocks_accountTransactionId>&& after,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  auto request = multiclient::RequestFunction<tonlib_api::blocks_getTransactionsExt>{
      .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
      .request_creator =
          [w = blk_id->workchain_,
           sh = blk_id->shard_,
           s = blk_id->seqno_,
           r = blk_id->root_hash_,
           f = blk_id->file_hash_,
           c = count] {
            std::int32_t mode = 7;
            return tonlib_api::make_object<tonlib_api::blocks_getTransactionsExt>(
                tonlib_api::make_object<tonlib_api::ton_blockIdExt>(w, sh, s, r, f), mode, c, nullptr
            );
          },
      .session = std::move(session)
  };
  if (after) {
    request.request_creator = [w = blk_id->workchain_,
                               sh = blk_id->shard_,
                               s = blk_id->seqno_,
                               r = blk_id->root_hash_,
                               f = blk_id->file_hash_,
                               a = after->account_,
                               l = after->lt_,
                               c = count] {
      std::int32_t mode = 7 + 128;
      return tonlib_api::make_object<tonlib_api::blocks_getTransactionsExt>(
          tonlib_api::make_object<tonlib_api::ton_blockIdExt>(w, sh, s, r, f),
          mode,
          c,
          tonlib_api::make_object<tonlib_api::blocks_accountTransactionId>(a, l)
      );
    };
  }
  auto [result, new_session] = send_request_function(std::move(request), !archival.has_value());
  return {std::move(result), new_session};
}
TonlibWorker::Result<tonlib_api::raw_getTransactions::ReturnType> TonlibWorker::raw_getTransactions(
    const std::string& account_address,
    const ton::LogicalTime& from_transaction_lt,
    const std::string& from_transaction_hash,
    const std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  auto request = multiclient::RequestFunction<tonlib_api::raw_getTransactions>{
      .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
      .request_creator =
          [a = account_address, fl = from_transaction_lt, fh = from_transaction_hash] {
            return tonlib_api::make_object<tonlib_api::raw_getTransactions>(
                nullptr,
                tonlib_api::make_object<tonlib_api::accountAddress>(a),
                tonlib_api::make_object<tonlib_api::internal_transactionId>(fl, fh)
            );
          },
      .session = std::move(session)
  };
  auto [result, new_session] = send_request_function(std::move(request), !archival.has_value());
  return {std::move(result), new_session};
}
TonlibWorker::Result<tonlib_api::raw_getTransactionsV2::ReturnType> TonlibWorker::raw_getTransactionsV2(
    const std::string& account_address,
    const ton::LogicalTime& from_transaction_lt,
    const std::string& from_transaction_hash,
    const size_t count,
    const bool try_decode_messages,
    const std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  auto request = multiclient::RequestFunction<tonlib_api::raw_getTransactionsV2>{
    .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
    .request_creator = [a = account_address, fl = from_transaction_lt, fh = from_transaction_hash, c = count, dm = try_decode_messages] {
      return tonlib_api::make_object<tonlib_api::raw_getTransactionsV2>(
        nullptr,
        tonlib_api::make_object<tonlib_api::accountAddress>(a),
        tonlib_api::make_object<tonlib_api::internal_transactionId>(fl, fh),
        c, dm
        );
    },
    .session = std::move(session)
  };
  auto [result, new_session] = send_request_function(std::move(request), !archival.has_value());
  return {std::move(result), new_session};
}
TonlibWorker::Result<tonlib_api::blocks_getTransactions::ReturnType> TonlibWorker::getBlockTransactions(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const size_t count,
    const std::string& root_hash,
    const std::string& file_hash,
    const std::optional<ton::LogicalTime>& after_lt,
    const std::string& after_hash,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  if (session == nullptr && archival.has_value()) {
    auto options = multiclient::RequestParameters{.mode = multiclient::RequestMode::Single, .archival = archival};
    auto r_session = tonlib_.get_session(options, nullptr);
    if (r_session.is_error()) {
      return {td::Status::Error(533, r_session.move_as_error_prefix("Failed to get session: ").message()), nullptr};
    }
    session = r_session.move_as_ok();
  }
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> blk_id = nullptr;
  if (!root_hash.empty() && !file_hash.empty()) {
    blk_id = tonlib_api::make_object<tonlib_api::ton_blockIdExt>(workchain, shard, seqno, root_hash, file_hash);
  } else {
    auto [r_blk_id, new_session] = lookupBlock(workchain, shard, seqno, std::nullopt, std::nullopt, session);
    if (r_blk_id.is_error()) {
      return {r_blk_id.move_as_error(), new_session};
    }
    blk_id = r_blk_id.move_as_ok();
    session = std::move(new_session);
  }

  tonlib_api::object_ptr<tonlib_api::blocks_accountTransactionId> after;
  if (after_lt.has_value()) {
    after = tonlib_api::make_object<tonlib_api::blocks_accountTransactionId>(after_hash, after_lt.value());
  } else {
    after = tonlib_api::make_object<tonlib_api::blocks_accountTransactionId>(
        utils::stringToHash("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=").value(), 0
    );
  }

  size_t left_count = count;
  bool is_finished = false;
  constexpr size_t CHUNK_SIZE = 256;

  tonlib_api::object_ptr<tonlib_api::blocks_transactions> txs =
      tonlib_api::make_object<tonlib_api::blocks_transactions>(
          nullptr, 0, true, std::move(std::vector<tonlib_api::object_ptr<tonlib_api::blocks_shortTxId>>{})
      );
  txs->transactions_.reserve(count);
  while (!is_finished) {
    size_t chunk_size = (left_count > CHUNK_SIZE ? CHUNK_SIZE : left_count);
    auto [result, new_session] = raw_getBlockTransactions(blk_id, chunk_size, std::move(after), archival, session);
    if (result.is_error()) {
      return {result.move_as_error(), new_session};
    }
    auto local = result.move_as_ok();
    session = std::move(new_session);

    txs->id_ = std::move(local->id_);
    txs->incomplete_ = local->incomplete_;
    // txs->req_count_ += local->transactions_.size();
    left_count -= local->transactions_.size();

    if (!local->transactions_.empty()) {
      auto last_idx = local->transactions_.size() - 1;
      after = tonlib_api::make_object<tonlib_api::blocks_accountTransactionId>(
          local->transactions_[last_idx]->account_, local->transactions_[last_idx]->lt_
      );
    }

    std::copy(
        std::make_move_iterator(local->transactions_.begin()),
        std::make_move_iterator(local->transactions_.end()),
        std::back_inserter(txs->transactions_)
    );
    is_finished = (left_count <= 0) || !local->incomplete_;
  }
  txs->req_count_ = static_cast<std::int32_t>(count);

  return {std::move(txs), session};
}
TonlibWorker::Result<tonlib_api::blocks_getTransactionsExt::ReturnType> TonlibWorker::getBlockTransactionsExt(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const size_t count,
    const std::string& root_hash,
    const std::string& file_hash,
    const std::optional<ton::LogicalTime>& after_lt,
    const std::string& after_hash,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  if (session == nullptr && archival.has_value()) {
    auto options = multiclient::RequestParameters{.mode = multiclient::RequestMode::Single, .archival = archival};
    auto r_session = tonlib_.get_session(options, nullptr);
    if (r_session.is_error()) {
      return {td::Status::Error(533, r_session.move_as_error_prefix("Failed to get session: ").message()), nullptr};
    }
    session = r_session.move_as_ok();
  }
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> blk_id = nullptr;
  if (!root_hash.empty() && !file_hash.empty()) {
    blk_id = tonlib_api::make_object<tonlib_api::ton_blockIdExt>(workchain, shard, seqno, root_hash, file_hash);
  } else {
    auto [r_blk_id, new_session] = lookupBlock(workchain, shard, seqno, std::nullopt, std::nullopt, session);
    if (r_blk_id.is_error()) {
      return {r_blk_id.move_as_error(), new_session};
    }
    blk_id = r_blk_id.move_as_ok();
    session = std::move(new_session);
  }

  tonlib_api::object_ptr<tonlib_api::blocks_accountTransactionId> after;
  if (after_lt.has_value()) {
    after = tonlib_api::make_object<tonlib_api::blocks_accountTransactionId>(after_hash, after_lt.value());
  } else {
    after = tonlib_api::make_object<tonlib_api::blocks_accountTransactionId>(
        utils::stringToHash("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=").value(), 0
    );
  }

  size_t left_count = count;
  bool is_finished = false;
  constexpr size_t CHUNK_SIZE = 256;

  tonlib_api::object_ptr<tonlib_api::blocks_transactionsExt> txs =
      tonlib_api::make_object<tonlib_api::blocks_transactionsExt>(
          nullptr, 0, true, std::move(std::vector<tonlib_api::object_ptr<tonlib_api::raw_transaction>>{})
      );
  txs->transactions_.reserve(count);
  while (!is_finished) {
    size_t chunk_size = (left_count > CHUNK_SIZE ? CHUNK_SIZE : left_count);
    auto [result, new_session] = raw_getBlockTransactionsExt(blk_id, chunk_size, std::move(after), archival, session);
    if (result.is_error()) {
      return {result.move_as_error(), new_session};
    }
    auto local = result.move_as_ok();
    session = std::move(new_session);

    txs->id_ = std::move(local->id_);
    txs->incomplete_ = local->incomplete_;
    // txs->req_count_ += local->transactions_.size();
    left_count -= local->transactions_.size();

    if (!local->transactions_.empty()) {
      auto last_idx = local->transactions_.size() - 1;
      auto std_address =
          block::StdAddress::parse(local->transactions_[last_idx]->address_->account_address_).move_as_ok();
      after = tonlib_api::make_object<tonlib_api::blocks_accountTransactionId>(
          std_address.addr.as_slice().str(), local->transactions_[last_idx]->transaction_id_->lt_
      );
    }

    std::copy(
        std::make_move_iterator(local->transactions_.begin()),
        std::make_move_iterator(local->transactions_.end()),
        std::back_inserter(txs->transactions_)
    );
    is_finished = (left_count <= 0) || !local->incomplete_;
  }
  txs->req_count_ = static_cast<std::int32_t>(count);
  return {std::move(txs), session};
}
TonlibWorker::Result<tonlib_api::raw_getTransactionsV2::ReturnType> TonlibWorker::getTransactions(
    const std::string& account_address,
    std::optional<ton::LogicalTime> from_transaction_lt,
    std::string from_transaction_hash,
    ton::LogicalTime to_transaction_lt,
    size_t count,
    size_t chunk_size,
    bool try_decode_messages,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  if (session == nullptr && archival.has_value()) {
    auto options = multiclient::RequestParameters{.mode = multiclient::RequestMode::Single, .archival = archival};
    auto r_session = tonlib_.get_session(options, nullptr);
    if (r_session.is_error()) {
      return {td::Status::Error(533, r_session.move_as_error_prefix("Failed to get session: ").message()), nullptr};
    }
    session = r_session.move_as_ok();
  }
  if (!(from_transaction_lt.has_value() && !from_transaction_hash.empty())) {
    auto [r_account_state_, new_session] = getAddressInformation(account_address, std::nullopt, session);
    if (r_account_state_.is_error()) {
      return {r_account_state_.move_as_error(), new_session};
    }
    session = std::move(new_session);
    auto account_state_ = r_account_state_.move_as_ok();
    from_transaction_lt = account_state_->last_transaction_id_->lt_;
    from_transaction_hash = account_state_->last_transaction_id_->hash_;
  }

  bool reach_lt = false;
  size_t tx_count = 0;
  auto current_lt = from_transaction_lt.value();
  auto current_hash = from_transaction_hash;

  tonlib_api::object_ptr<tonlib_api::raw_transactions> txs = tonlib_api::make_object<tonlib_api::raw_transactions>();
  while (!reach_lt && tx_count < count) {
    size_t local_chunk_size = (count - tx_count > chunk_size ? chunk_size : count - tx_count);
    auto [r_local, new_session] = raw_getTransactionsV2(
        account_address, current_lt, current_hash, local_chunk_size, try_decode_messages, archival, session
    );
    if (r_local.is_error()) {
      return {r_local.move_as_error(), new_session};
    }
    auto local = r_local.move_as_ok();
    session = std::move(new_session);

    for (auto& tx : local->transactions_) {
      if (tx->transaction_id_->lt_ <= to_transaction_lt) {
        reach_lt = true;
      } else {
        ++tx_count;
      }
    }

    // it seems that previous_transaction_id_ is always not nullptr, however I'll leave it as it was in Python version
    if (auto& next_tx = local->previous_transaction_id_; next_tx) {
      current_lt = next_tx->lt_;
      current_hash = next_tx->hash_;
    } else {
      reach_lt = true;
    }
    if (current_lt == 0) {
      reach_lt = true;
    }
    std::copy(
        std::move_iterator(local->transactions_.begin()),
        std::move_iterator(local->transactions_.end()),
        std::back_inserter(txs->transactions_)
    );
    if (local->previous_transaction_id_) {
      txs->previous_transaction_id_ = std::move(local->previous_transaction_id_);
    }
  }
  if (txs->transactions_.size() > tx_count) {
    txs->transactions_.resize(tx_count);
  }
  return {std::move(txs), session};
}

TonlibWorker::Result<tonlib_api::raw_getTransactionsV2::ReturnType> TonlibWorker::tryLocateTransactionByIncomingMessage(
    const std::string& source,
    const std::string& destination,
    ton::LogicalTime created_lt,
    multiclient::SessionPtr session
) const {
  auto r_src_addr = block::StdAddress::parse(source);
  if (r_src_addr.is_error()) {
    return {r_src_addr.move_as_error_prefix("failed to parse source: "), session};
  }
  auto src = r_src_addr.move_as_ok();

  auto r_dest_addr = block::StdAddress::parse(destination);
  if (r_dest_addr.is_error()) {
    return {r_dest_addr.move_as_error_prefix("failed to parse destination: "), session};
  }
  auto dest = r_dest_addr.move_as_ok();

  auto workchain = dest.workchain;
  auto [r_shards, new_session] = getShards(std::nullopt, created_lt, std::nullopt, session);
  if (r_shards.is_error()) {
    return {r_shards.move_as_error_prefix("failed to get shards at create_lt: "), new_session};
  }
  auto shards = r_shards.move_as_ok();
  for (auto& shard : shards->shards_) {
    auto shard_id = shard->shard_;
    for (auto i = 0; i < 3; ++i) {
      auto lt = created_lt + 1000000 * i;
      auto [r_block, new_session] = lookupBlock(workchain, shard_id, std::nullopt, lt, std::nullopt, session);
      if (r_block.is_error()) {
        td::StringBuilder sb;
        sb << "failed to lookup block with lt " << lt << ": ";
        return {r_block.move_as_error_prefix(sb.as_cslice().str()), new_session};
      }
      auto block = r_block.move_as_ok();
      session = std::move(new_session);

      constexpr size_t tx_count = 40;
      auto [r_txs, new_session_2] = getBlockTransactions(
          block->workchain_, block->shard_, block->seqno_, tx_count, block->root_hash_, block->file_hash_, 40, "", std::nullopt, session
      );
      if (r_txs.is_error()) {
        td::StringBuilder sb;
        sb << "failed to get transactions for block (" << block->workchain_ << ", " << block->shard_ << ", "
           << block->seqno_ << "): ";
        return {r_txs.move_as_error_prefix(sb.as_cslice().str()), new_session_2};
      }
      auto blk_txs = r_txs.move_as_ok();
      session = std::move(new_session_2);

      tonlib_api::object_ptr<tonlib_api::blocks_shortTxId> candidate = nullptr;
      size_t tx_found = 0;
      for (auto& tx : blk_txs->transactions_) {
        auto tx_addr_str = std::to_string(block->workchain_) + ":" + td::hex_encode(tx->account_);
        auto tx_addr = block::StdAddress::parse(tx_addr_str).move_as_ok();
        if (dest.addr == tx_addr.addr && (candidate == nullptr || candidate->lt_ < tx->lt_)) {
          ++tx_found;
          candidate = std::move(tx);
        }
      }
      if (candidate != nullptr) {
        constexpr size_t min_tx_found = 10;
        auto [r_candidate_txs, new_session] =
            getTransactions(destination, candidate->lt_, candidate->hash_, std::max(tx_found, min_tx_found), 10, 30, true, std::nullopt, session);
        if (r_txs.is_error()) {
          return {r_txs.move_as_error_prefix("failed to get candidate transactions: "), new_session};
        }
        auto candidate_txs = r_candidate_txs.move_as_ok();
        session = std::move(new_session);
        for (auto & candidate_tx : candidate_txs->transactions_) {
          auto& in_msg = candidate_tx->in_msg_;
          if (in_msg && in_msg->source_ && !in_msg->source_->account_address_.empty()) {
            auto tx_src_addr = block::StdAddress::parse(in_msg->source_->account_address_).move_as_ok();
            if (src.workchain == tx_src_addr.workchain && src.addr == tx_src_addr.addr && in_msg->created_lt_ == created_lt) {
              std::vector<tonlib_api::object_ptr<tonlib_api::raw_transaction>> tx_vec;
              tx_vec.emplace_back(std::move(candidate_tx));
              auto prev_tx = tonlib_api::make_object<tonlib_api::internal_transactionId>(
                0, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA="
              );
              return {tonlib_api::make_object<tonlib_api::raw_transactions>(std::move(tx_vec), std::move(prev_tx)), session};
            }
          }
        }
      }
    }
  }
  return {td::Status::Error(404, "transaction was not found"), session};
}

TonlibWorker::Result<tonlib_api::raw_getTransactionsV2::ReturnType> TonlibWorker::tryLocateTransactionByOutgoingMessage(
    const std::string& source,
    const std::string& destination,
    ton::LogicalTime created_lt,
    multiclient::SessionPtr session
) const {
  auto r_src_addr = block::StdAddress::parse(source);
  if (r_src_addr.is_error()) {
    return {r_src_addr.move_as_error_prefix("failed to parse source: "), session};
  }
  auto src = r_src_addr.move_as_ok();

  auto r_dest_addr = block::StdAddress::parse(destination);
  if (r_dest_addr.is_error()) {
    return {r_dest_addr.move_as_error_prefix("failed to parse destination: "), session};
  }
  auto dest = r_dest_addr.move_as_ok();

  auto workchain = src.workchain;
  auto [r_shards, new_session] = getShards(std::nullopt, created_lt, std::nullopt, session);
  session = std::move(new_session);
  if (r_shards.is_error()) {
    return {r_shards.move_as_error_prefix("failed to get shards at create_lt: "), session};
  }
  auto shards = r_shards.move_as_ok();
  for (auto& shard : shards->shards_) {
    auto shard_id = shard->shard_;
    auto [r_block, new_session] = lookupBlock(workchain, shard_id, std::nullopt, created_lt, std::nullopt, session);
    session = std::move(new_session);
    if (r_block.is_error()) {
      td::StringBuilder sb;
      sb << "failed to lookup block with lt " << created_lt << ": ";
      return {r_block.move_as_error_prefix(sb.as_cslice().str()), session};
    }
    auto block = r_block.move_as_ok();

    constexpr size_t tx_count = 40;
    auto [r_txs, new_session_2] = getBlockTransactions(
        block->workchain_, block->shard_, block->seqno_, tx_count, block->root_hash_, block->file_hash_, std::nullopt, "", std::nullopt, session
    );
    session = std::move(new_session_2);
    if (r_txs.is_error()) {
      td::StringBuilder sb;
      sb << "failed to get transactions for block (" << block->workchain_ << ", " << block->shard_ << ", "
         << block->seqno_ << "): ";
      return {r_txs.move_as_error_prefix(sb.as_cslice().str()), session};
    }
    auto blk_txs = r_txs.move_as_ok();

    tonlib_api::object_ptr<tonlib_api::blocks_shortTxId> candidate = nullptr;
    size_t tx_found = 0;
    for (auto& tx : blk_txs->transactions_) {
      auto tx_addr_str = std::to_string(block->workchain_) + ":" + td::hex_encode(tx->account_);
      auto tx_addr = block::StdAddress::parse(tx_addr_str).move_as_ok();
      if (src.addr == tx_addr.addr && (candidate == nullptr || candidate->lt_ < tx->lt_)) {
        ++tx_found;
        candidate = std::move(tx);
      }
    }
    if (candidate != nullptr) {
      constexpr size_t min_tx_found = 10;
      auto [r_candidate_txs, new_session] =
          getTransactions(source, candidate->lt_, candidate->hash_, std::max(tx_found, min_tx_found), 10, 30, true, std::nullopt, session);
      session = std::move(new_session);
      if (r_txs.is_error()) {
        return {r_txs.move_as_error_prefix("failed to get candidate transactions: "), session};
      }
      auto candidate_txs = r_candidate_txs.move_as_ok();
      for (auto& candidate_tx : candidate_txs->transactions_) {
        for (auto& out_msg : candidate_tx->out_msgs_) {
          if (out_msg && out_msg->destination_ && !out_msg->destination_->account_address_.empty()) {
            auto tx_dest_addr = block::StdAddress::parse(out_msg->destination_->account_address_).move_as_ok();
            if (dest.workchain == tx_dest_addr.workchain && dest.addr == tx_dest_addr.addr &&
                out_msg->created_lt_ == created_lt) {
              std::vector<tonlib_api::object_ptr<tonlib_api::raw_transaction>> tx_vec;
              tx_vec.emplace_back(std::move(candidate_tx));
              auto prev_tx = tonlib_api::make_object<tonlib_api::internal_transactionId>(
                  0, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA="
              );
              return {tonlib_api::make_object<tonlib_api::raw_transactions>(std::move(tx_vec), std::move(prev_tx)), session};
            }
          }
        }
      }
    }
  }
  return {td::Status::Error(404, "transaction was not found"), session};
}

TonlibWorker::Result<tonlib_api::raw_sendMessage::ReturnType> TonlibWorker::raw_sendMessage(
  const std::string& boc,
  multiclient::SessionPtr session
) const {
  auto r_boc = td::base64_decode(boc);
  if (r_boc.is_error()) {
    return {r_boc.move_as_error(), session};
  }
  auto boc_bytes = r_boc.move_as_ok();
  auto request = multiclient::RequestFunction<tonlib_api::raw_sendMessage>{
    .parameters = {.mode=multiclient::RequestMode::Multiple, .clients_number = 5},
    .request_creator = [boc_bytes]() {
      return tonlib_api::make_object<tonlib_api::raw_sendMessage>(boc_bytes);
    },
    .session = session
  };
  auto [result, new_session] = send_request_function(std::move(request), false);
  return {std::move(result), new_session};
}

TonlibWorker::Result<tonlib_api::raw_sendMessageReturnHash::ReturnType> TonlibWorker::raw_sendMessageReturnHash(
    const std::string& boc, multiclient::SessionPtr session
) const {
  auto r_boc = td::base64_decode(boc);
  if (r_boc.is_error()) {
    return {r_boc.move_as_error(), session};
  }
  auto boc_bytes = r_boc.move_as_ok();
  auto request = multiclient::RequestFunction<tonlib_api::raw_sendMessageReturnHash>{
      .parameters = {.mode = multiclient::RequestMode::Multiple, .clients_number = 5},
      .request_creator =
          [boc_bytes]() { return tonlib_api::make_object<tonlib_api::raw_sendMessageReturnHash>(boc_bytes); },
      .session = session
  };
  auto [result, new_session] = send_request_function(std::move(request), false);
  return {std::move(result), new_session};
}

TonlibWorker::Result<std::unique_ptr<tonlib_api::smc_info>> TonlibWorker::loadContract(
    const std::string& address,
    std::optional<ton::BlockSeqno> seqno,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  if (session == nullptr && archival.has_value()) {
    auto options = multiclient::RequestParameters{.mode = multiclient::RequestMode::Single, .archival = archival};
    auto r_session = tonlib_.get_session(options, nullptr);
    if (r_session.is_error()) {
      return {td::Status::Error(533, r_session.move_as_error_prefix("Failed to get session: ").message()), nullptr};
    }
    session = r_session.move_as_ok();
  }
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> with_block;
  if (seqno.has_value()) {
    auto [res, new_session] =
        lookupBlock(ton::masterchainId, ton::shardIdAll, seqno.value(), std::nullopt, std::nullopt, session);
    session = std::move(new_session);
    if (!res.is_ok()) {
      return {res.move_as_error(), session};
    }
    with_block = res.move_as_ok();
  }

  if (!with_block) {
    auto request = multiclient::RequestFunction<tonlib_api::smc_load>{
        .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
        .request_creator =
            [address]() {
              return tonlib_api::make_object<tonlib_api::smc_load>(
                  tonlib_api::make_object<tonlib_api::accountAddress>(address)
              );
            },
        .session = session
    };
    auto [result, new_session] = send_request_function(std::move(request), true);
    return {std::move(result), std::move(new_session)};
  }
  auto request = multiclient::RequestFunction<tonlib_api::withBlock>{
      .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
      .request_creator =
          [address_ = address,
           workchain_ = with_block->workchain_,
           shard_ = with_block->shard_,
           seqno_ = with_block->seqno_,
           root_hash_ = with_block->root_hash_,
           file_hash_ = with_block->file_hash_] {
            return tonlib_api::make_object<tonlib_api::withBlock>(
                tonlib_api::make_object<tonlib_api::ton_blockIdExt>(workchain_, shard_, seqno_, root_hash_, file_hash_),
                tonlib_api::make_object<tonlib_api::smc_load>(
                    tonlib_api::make_object<tonlib_api::accountAddress>(address_)
                )
            );
          },
      .session = session
  };
  auto [result, new_session] = send_request_function(std::move(request), false);
  if (result.is_error()) {
    return {result.move_as_error(), new_session};
  }
  return {ton::move_tl_object_as<tonlib_api::smc_info>(result.move_as_ok()), new_session};
}
TonlibWorker::Result<RunGetMethodResult> TonlibWorker::runGetMethod(
    const std::string& address,
    const std::string& method_name,
    const std::string& stack,
    std::optional<ton::BlockSeqno> seqno,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  auto [r_smc_info, new_session] = loadContract(address, seqno, archival, session);
  session = std::move(new_session);
  if (!r_smc_info.is_ok()) {
    return {r_smc_info.move_as_error(), session};
  }
  auto smc_info = r_smc_info.move_as_ok();
  auto r_stack = tvm::parse_stack(stack);
  if (r_stack.is_error()) {
    return {r_stack.move_as_error(), session};
  }
  auto request = multiclient::RequestFunction<tonlib_api::smc_runGetMethod>{
    .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
    .request_creator =
        [id_ = smc_info->id_, method_name_ = method_name, stack_str = stack] {
          auto method_int = td::string_to_int256(method_name_);
          auto stack = tvm::parse_stack(stack_str).move_as_ok();
          if (method_int.is_null()) {
            return tonlib_api::make_object<tonlib_api::smc_runGetMethod>(
                id_, tonlib_api::make_object<tonlib_api::smc_methodIdName>(method_name_), std::move(stack)
            );
          }
          return tonlib_api::make_object<tonlib_api::smc_runGetMethod>(
              id_, tonlib_api::make_object<tonlib_api::smc_methodIdNumber>(method_int->to_long()), std::move(stack)
          );
    },
  .session = session
  };
  auto [result, new_session_2] = send_request_function(std::move(request), false);
  session = std::move(new_session_2);
  if (result.is_error()) {
    return {result.move_as_error(), session};
  }

  auto state_request = multiclient::RequestFunction<tonlib_api::smc_getRawFullAccountState>{
    .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
    .request_creator =
        [id_ = smc_info->id_] { return tonlib_api::make_object<tonlib_api::smc_getRawFullAccountState>(id_); },
    .session = session
};
  auto [state_result, new_session_3] = send_request_function(std::move(state_request), false);
  session = std::move(new_session_3);
  if (state_result.is_error()) {
    return {state_result.move_as_error(), session};
  }

  // TODO: call smc.forget to avoid ram overload

  return {RunGetMethodResult{result.move_as_ok(), state_result.move_as_ok()}, session};
}

TonlibWorker::Result<std::unique_ptr<tonlib_api::query_fees>> TonlibWorker::queryEstimateFees(
    const std::string& account_address,
    const std::string& body,
    const std::string& init_code,
    const std::string& init_data,
    bool ignore_chksig,
    multiclient::SessionPtr session
) const {
  auto r_body = td::base64_decode(body);
  if (r_body.is_error()) {
    return {r_body.move_as_error(), session};
  }
  auto body_bin = r_body.move_as_ok();

  auto r_init_code = td::base64_decode(init_code);
  if (init_code.length() > 0 && r_init_code.is_error()) {
    return {r_init_code.move_as_error(), session};
  }
  auto init_code_bin = r_init_code.move_as_ok();

  auto r_init_data = td::base64_decode(init_data);
  if (init_data.length() > 0 && r_init_data.is_error()) {
    return {r_init_data.move_as_error(), session};
  }
  auto init_data_bin = r_init_data.move_as_ok();

  auto request = multiclient::RequestFunction<tonlib_api::raw_createQuery>{
      .parameters = {.mode = multiclient::RequestMode::Single, .archival = std::nullopt},
      .request_creator =
          [addr_ = account_address, init_code_ = init_code_bin, init_data_ = init_data_bin, body_ = body_bin] {
            return tonlib_api::make_object<tonlib_api::raw_createQuery>(
                tonlib_api::make_object<tonlib_api::accountAddress>(addr_), init_code_, init_data_, body_
            );
          },
      .session = session
  };
  auto [result, new_session] = send_request_function(std::move(request), false);
  session = std::move(new_session);
  if (result.is_error()) {
    return {result.move_as_error(), session};
  }
  auto query_info = result.move_as_ok();

  auto request_2 = multiclient::RequestFunction<tonlib_api::query_estimateFees>{
      .parameters = {.mode = multiclient::RequestMode::Single, .archival = std::nullopt},
      .request_creator =
          [id = query_info->id_, ignore_chksig_ = ignore_chksig] {
            return tonlib_api::make_object<tonlib_api::query_estimateFees>(id, ignore_chksig_);
          },
      .session = std::move(session)
  };
  auto [result_2, new_session_2] = send_request_function(std::move(request_2), false);
  session = std::move(new_session_2);
  if (result_2.is_error()) {
    return {result_2.move_as_error(), session};
  }
  return {result_2.move_as_ok(), session};
}

TonlibWorker::Result<std::unique_ptr<TokenDataResult>> TonlibWorker::checkJettonMaster(
    const std::string& address,
    bool skip_verification,
    std::optional<ton::BlockSeqno> seqno,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  auto [r_smc_info, new_session] = loadContract(address, seqno, archival, session);
  session = std::move(new_session);
  if (!r_smc_info.is_ok()) {
    return {r_smc_info.move_as_error(), session};
  }
  auto smc_info = r_smc_info.move_as_ok();

  std::string method_name = "get_jetton_data";
  auto request = multiclient::RequestFunction<tonlib_api::smc_runGetMethod>{
    .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
    .request_creator = [id_ = smc_info->id_, method_ = method_name] {
      return tonlib_api::make_object<tonlib_api::smc_runGetMethod>(id_,
        tonlib_api::make_object<tonlib_api::smc_methodIdName>(method_),
        std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>{});
    },
    .session = std::move(session)
  };
  auto [res, new_session_2] = send_request_function(std::move(request), false);
  session = std::move(new_session_2);
  if (!res.is_ok()) {
    return {nullptr, std::move(session)};
  }
  auto result = res.move_as_ok();
  if (result->exit_code_ != 0) {
    return {nullptr, std::move(session)};
  }
  auto data = std::make_unique<JettonMasterDataResult>(address);

  // total_supply_
  auto r_total_supply = tvm::number_from_tvm_stack_entry(result->stack_[0]);
  if (r_total_supply.is_error()) {
    return {r_total_supply.move_as_error(), std::move(session)};
  }
  data->total_supply_ = r_total_supply.move_as_ok();

  // mintable_
  auto r_mintable = tvm::number_from_tvm_stack_entry(result->stack_[1]);
  if (r_mintable.is_error()) {
    return {r_mintable.move_as_error(), std::move(session)};
  }
  data->mintable_ = std::stoi(r_mintable.move_as_ok());

  // admin_address_
  auto r_admin_address = tvm::address_from_tvm_stack_entry(result->stack_[2]);
  if (r_admin_address.is_error()) {
    return {r_admin_address.move_as_error(), std::move(session)};
  }
  data->admin_address_ = r_admin_address.move_as_ok();

  // jetton_content_
  if (result->stack_[3]->get_id() != tonlib_api::tvm_stackEntryCell::ID) {
    return {td::Status::Error(500, "stackEntryCell expected at 3 position"), std::move(session)};
  }
  auto r_jetton_content_cell_data = static_cast<tonlib_api::tvm_stackEntryCell&>(*result->stack_[3]).cell_->bytes_;
  auto r_jetton_content_cell = vm::std_boc_deserialize(r_jetton_content_cell_data, true, true);
  if (r_jetton_content_cell.is_error()) {
    return {r_jetton_content_cell.move_as_error_prefix("Failed to parse jetton content cell: "), std::move(session)};
  }
  auto jetton_content_cell = r_jetton_content_cell.move_as_ok();

  auto r_jetton_content = tvm::parse_token_data(std::move(jetton_content_cell));
  if (r_jetton_content.is_error()) {
    return {r_jetton_content.move_as_error_prefix("Failed to parse jetton content from the cell: "), std::move(session)};
  }
  auto [jetton_content_onchain, jetton_content] = r_jetton_content.move_as_ok();
  data->jetton_content_onchain_ = jetton_content_onchain;
  data->jetton_content_ = std::move(jetton_content);

  // jetton_wallet_code_
  if (result->stack_[4]->get_id() != tonlib_api::tvm_stackEntryCell::ID) {
    return {td::Status::Error(500, "stackEntryCell expected at 3 position"), std::move(session)};
  }
  data->jetton_wallet_code_ = td::base64_encode(static_cast<const tonlib_api::tvm_stackEntryCell&>(*result->stack_[4]).cell_->bytes_);

  return {std::move(data), std::move(session)};
}
TonlibWorker::Result<std::unique_ptr<TokenDataResult>> TonlibWorker::checkJettonWallet(
    const std::string& address,
    bool skip_verification,
    std::optional<ton::BlockSeqno> seqno,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  auto [r_smc_info, new_session] = loadContract(address, seqno, archival, session);
  session = std::move(new_session);
  if (!r_smc_info.is_ok()) {
    return {r_smc_info.move_as_error(), session};
  }
  auto smc_info = r_smc_info.move_as_ok();

  std::string method_name = "get_wallet_data";
  auto request = multiclient::RequestFunction<tonlib_api::smc_runGetMethod>{
    .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
    .request_creator = [id_ = smc_info->id_, method_ = method_name] {
      return tonlib_api::make_object<tonlib_api::smc_runGetMethod>(id_,
        tonlib_api::make_object<tonlib_api::smc_methodIdName>(method_),
        std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>{});
    },
    .session = std::move(session)
  };
  auto [res, new_session_2] = send_request_function(std::move(request), false);
  session = std::move(new_session_2);
  if (!res.is_ok()) {
    return {nullptr, std::move(session)};
  }
  auto result = res.move_as_ok();
  if (result->exit_code_ != 0) {
    return {nullptr, std::move(session)};
  }
  auto data = std::make_unique<JettonWalletDataResult>(address);

  // balance_
  auto r_balance = tvm::number_from_tvm_stack_entry(result->stack_[0]);
  if (r_balance.is_error()) {
    return {r_balance.move_as_error(), std::move(session)};
  }
  data->balance_ = r_balance.move_as_ok();

  // owner_address_
  auto r_owner_address_ = tvm::address_from_tvm_stack_entry(result->stack_[1]);
  if (r_owner_address_.is_error()) {
    return {r_owner_address_.move_as_error(), std::move(session)};
  }
  data->owner_address_ = r_owner_address_.move_as_ok();

  // jetton_master_address_
  auto r_jetton_master_address = tvm::address_from_tvm_stack_entry(result->stack_[2]);
  if (r_jetton_master_address.is_error()) {
    return {r_jetton_master_address.move_as_error(), std::move(session)};
  }
  data->jetton_master_address_ = r_jetton_master_address.move_as_ok();

  // jetton_wallet_code_
  if (result->stack_[3]->get_id() != tonlib_api::tvm_stackEntryCell::ID) {
    return {td::Status::Error(500, "stackEntryCell expected at 3 position"), std::move(session)};
  }
  data->jetton_wallet_code_ = td::base64_encode(static_cast<const tonlib_api::tvm_stackEntryCell&>(*result->stack_[3]).cell_->bytes_);

  if (skip_verification) {
    return {std::move(data), std::move(session)};
  }

  // verification
  auto [r_parent_smc_info, new_session_3] = loadContract(data->jetton_master_address_, seqno, archival, session);
  session = std::move(new_session_3);
  if (!r_parent_smc_info.is_ok()) {
    return {r_parent_smc_info.move_as_error(), session};
  }
  auto parent_smc_info = r_parent_smc_info.move_as_ok();

  // construct owner_address cell
  auto r_owner_std_address = block::StdAddress::parse(data->owner_address_);
  if (r_owner_std_address.is_error()) {
    return {r_owner_std_address.move_as_error(), std::move(session)};
  }
  auto owner_address_std = r_owner_std_address.move_as_ok();

  vm::CellBuilder anycast_cb;
  anycast_cb.store_bool_bool(false);
  auto anycast_cell = anycast_cb.finalize();
  td::Ref<vm::CellSlice> anycast_cs = vm::load_cell_slice_ref(anycast_cell);

  vm::CellBuilder cb;
  block::gen::t_MsgAddressInt.pack_addr_std(cb, anycast_cs, owner_address_std.workchain, owner_address_std.addr);
  auto r_owner_address_cell = vm::std_boc_serialize(cb.finalize());
  if (r_owner_address_cell.is_error()) {
    return {r_owner_address_cell.move_as_error(), std::move(session)};
  }
  auto owner_address_cell = r_owner_address_cell.move_as_ok().as_slice().str();

  // call get_wallet_address on master
  auto request_2 = multiclient::RequestFunction<tonlib_api::smc_runGetMethod>{
    .parameters = {.mode=multiclient::RequestMode::Single, .archival = archival},
    .request_creator = [id_ = parent_smc_info->id_, owner_address_cell_ = owner_address_cell] {
      std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>> stack;
      auto tonlib_slice = tonlib_api::make_object<tonlib_api::tvm_slice>(owner_address_cell_);
      auto entry = tonlib_api::make_object<tonlib_api::tvm_stackEntrySlice>(std::move(tonlib_slice));
      stack.push_back(std::move(entry));
      return tonlib_api::make_object<tonlib_api::smc_runGetMethod>(
        id_,
        tonlib_api::make_object<tonlib_api::smc_methodIdName>("get_wallet_address"),
        std::move(stack));
    },
    .session = std::move(session),
  };
  auto [res_2, new_session_4] = send_request_function(std::move(request_2));
  session = std::move(new_session_4);
  if (!res_2.is_ok()) {
    auto error = res_2.move_as_error();
    return {std::move(error), std::move(session)};
  }
  auto result_2 = res_2.move_as_ok();
  if (result_2->exit_code_ != 0) {
    LOG(ERROR) << "Exit code " << result_2->exit_code_ << " != 0";
    return {nullptr, std::move(session)};
  }
  auto& entry = static_cast<tonlib_api::tvm_stackEntryCell&>(*(result_2->stack_[0]));

  // {
  //   auto r_cell = vm::std_boc_deserialize(entry.cell_->bytes_, true, true);
  //   if (r_cell.is_error()) {
  //     LOG(ERROR) << "load cell failed: " << r_cell.move_as_error();
  //   }
  //   // te6ccgEBAQEAJAAAQ4AW+iDq6ufjgt/MXgt/cX7iXPwWGOqiQ0OU+vlAGoMXuRA=
  //   // te6cckEBAQEAJAAAQ4AZHxqOGXySrVi9jb4K0H8twOHNPsDa7RRMlejtyfOKmvAVy/1x
  //   auto cell = r_cell.move_as_ok();
  //   auto cs = vm::load_cell_slice_ref(cell);
  //   auto tag = block::gen::MsgAddressInt().get_tag(*cs);
  //   if (tag < 0) {
  //     LOG(ERROR) << td::Status::Error("Failed to read MsgAddressInt tag");
  //   }
  //   if (tag != block::gen::MsgAddressInt::addr_std) {
  //     LOG(ERROR) << "non addr_std";
  //   }
  //   block::gen::MsgAddressInt::Record_addr_std addr;
  //   if (!tlb::csr_unpack(cs, addr)) {
  //     LOG(ERROR) << td::Status::Error("Failed to unpack MsgAddressInt");
  //   }
  //   LOG(ERROR) << block::StdAddress(addr.workchain_id, addr.address);
  // }

  auto r_wallet_address_from_master = tvm::address_from_tvm_stack_entry(result_2->stack_[0]);
  if (r_wallet_address_from_master.is_error()) {
    return {r_wallet_address_from_master.move_as_error(), std::move(session)};
  }
  auto wallet_address_from_master = r_wallet_address_from_master.move_as_ok();

  // check addresses
  auto r_address_std = block::StdAddress::parse(address);
  if (r_address_std.is_error()) {
    return {r_address_std.move_as_error(), std::move(session)};
  }
  auto address_std = r_address_std.move_as_ok();

  auto r_address_from_master_std = block::StdAddress::parse(wallet_address_from_master);
  if (r_address_from_master_std.is_error()) {
    return {r_address_from_master_std.move_as_error(), std::move(session)};
  }
  auto address_from_master_std = r_address_from_master_std.move_as_ok();

  LOG(DEBUG) << "address: " << address_std << " expected: " << address_from_master_std;
  if (address_from_master_std != address_std) {
    return {td::Status::Error(409, "Verification on master failed"), std::move(session)};
  }
  data->is_validated_ = true;

  return {std::move(data), std::move(session)};
}
TonlibWorker::Result<std::unique_ptr<TokenDataResult>> TonlibWorker::checkNFTCollection(
    const std::string& address,
    bool skip_verification,
    std::optional<ton::BlockSeqno> seqno,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  auto [r_smc_info, new_session] = loadContract(address, seqno, archival, session);
  session = std::move(new_session);
  if (!r_smc_info.is_ok()) {
    return {r_smc_info.move_as_error(), session};
  }
  auto smc_info = r_smc_info.move_as_ok();

  std::string method_name = "get_collection_data";
  auto request = multiclient::RequestFunction<tonlib_api::smc_runGetMethod>{
    .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
    .request_creator = [id_ = smc_info->id_, method_ = method_name] {
      return tonlib_api::make_object<tonlib_api::smc_runGetMethod>(id_,
        tonlib_api::make_object<tonlib_api::smc_methodIdName>(method_),
        std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>{});
    },
    .session = std::move(session)
  };
  auto [res, new_session_2] = send_request_function(std::move(request), false);
  session = std::move(new_session_2);
  if (!res.is_ok()) {
    return {nullptr, std::move(session)};
  }
  auto result = res.move_as_ok();
  if (result->exit_code_ != 0) {
    return {nullptr, std::move(session)};
  }
  auto data = std::make_unique<NFTCollectionDataResult>(address);

  // total_supply_
  if (result->stack_[0]->get_id() != tonlib_api::tvm_stackEntryNumber::ID) {
    return {td::Status::Error(500, "stackEntryNumber expected at 0 position"), std::move(session)};
  }
  auto r_next_item_index = tvm::number_from_tvm_stack_entry(result->stack_[0]);
  if (r_next_item_index.is_error()) {
    return {r_next_item_index.move_as_error(), std::move(session)};
  }
  data->next_item_index_ = r_next_item_index.move_as_ok();

  // collection_content_
  if (result->stack_[1]->get_id() != tonlib_api::tvm_stackEntryCell::ID) {
    return {td::Status::Error(500, "stackEntryCell expected at 1 position"), std::move(session)};
  }
  auto r_collection_content_cell_data = static_cast<tonlib_api::tvm_stackEntryCell&>(*result->stack_[1]).cell_->bytes_;
  auto r_collection_content_cell = vm::std_boc_deserialize(r_collection_content_cell_data, true, true);
  if (r_collection_content_cell.is_error()) {
    return {r_collection_content_cell.move_as_error_prefix("Failed to parse jetton content cell: "), std::move(session)};
  }
  auto collection_content_cell = r_collection_content_cell.move_as_ok();

  auto r_collection_content = tvm::parse_token_data(std::move(collection_content_cell));
  if (r_collection_content.is_error()) {
    return {r_collection_content.move_as_error_prefix("Failed to parse jetton content from the cell: "), std::move(session)};
  }
  auto [collection_content_onchain, collection_content] = r_collection_content.move_as_ok();
  data->collection_content_onchain_ = collection_content_onchain;
  data->collection_content_ = std::move(collection_content);

  // owner_address_
  auto r_owner_address_ = tvm::address_from_tvm_stack_entry(result->stack_[2]);
  if (r_owner_address_.is_error()) {
    return {r_owner_address_.move_as_error(), std::move(session)};
  }
  data->owner_address_ = r_owner_address_.move_as_ok();

  return {std::move(data), std::move(session)};
}
TonlibWorker::Result<std::unique_ptr<TokenDataResult>> TonlibWorker::checkNFTItem(
    const std::string& address,
    bool skip_verification,
    std::optional<ton::BlockSeqno> seqno,
    std::optional<bool> archival,
    multiclient::SessionPtr session
) const {
  auto [r_smc_info, new_session] = loadContract(address, seqno, archival, session);
  session = std::move(new_session);
  if (!r_smc_info.is_ok()) {
    return {r_smc_info.move_as_error(), session};
  }
  auto smc_info = r_smc_info.move_as_ok();

  std::string method_name = "get_nft_data";
  auto request = multiclient::RequestFunction<tonlib_api::smc_runGetMethod>{
    .parameters = {.mode = multiclient::RequestMode::Single, .archival = archival},
    .request_creator = [id_ = smc_info->id_, method_ = method_name] {
      return tonlib_api::make_object<tonlib_api::smc_runGetMethod>(id_,
        tonlib_api::make_object<tonlib_api::smc_methodIdName>(method_),
        std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>{});
    },
    .session = std::move(session)
  };
  auto [res, new_session_2] = send_request_function(std::move(request), false);
  session = std::move(new_session_2);
  if (!res.is_ok()) {
    return {nullptr, std::move(session)};
  }
  auto result = res.move_as_ok();
  if (result->exit_code_ != 0) {
    return {nullptr, std::move(session)};
  }
  auto data = std::make_unique<NFTItemDataResult>(address);

  // init_
  auto r_init = tvm::number_from_tvm_stack_entry(result->stack_[0]);
  if (r_init.is_error()) {
    return {r_init.move_as_error(), std::move(session)};
  }
  data->init_ = std::stoi(r_init.move_as_ok());


  // index_
  auto r_index = tvm::number_from_tvm_stack_entry(result->stack_[1]);
  if (r_index.is_error()) {
    return {r_index.move_as_error(), std::move(session)};
  }
  data->index_ = r_index.move_as_ok();

  // collection_address_
  auto r_collection_address = tvm::address_from_tvm_stack_entry(result->stack_[2]);
  if (r_collection_address.is_error()) {
    return {r_collection_address.move_as_error(), std::move(session)};
  }
  data->collection_address_ = r_collection_address.move_as_ok();

  // owner_address_
  auto r_owner_address_ = tvm::address_from_tvm_stack_entry(result->stack_[3]);
  if (r_owner_address_.is_error()) {
    return {r_owner_address_.move_as_error(), std::move(session)};
  }
  data->owner_address_ = r_owner_address_.move_as_ok();

  // content_
  if (result->stack_[4]->get_id() != tonlib_api::tvm_stackEntryCell::ID) {
    return {td::Status::Error(500, "stackEntryCell expected at 4 position"), std::move(session)};
  }
  auto ind_content_cell_data = static_cast<tonlib_api::tvm_stackEntryCell&>(*result->stack_[4]).cell_->bytes_;

  if (data->collection_address_.empty()) {
    auto r_content_cell = vm::std_boc_deserialize(ind_content_cell_data, true, true);
    if (r_content_cell.is_error()) {
      return {r_content_cell.move_as_error_prefix("Failed to parse jetton content cell: "), std::move(session)};
    }
    auto content_cell = r_content_cell.move_as_ok();

    auto r_content = tvm::parse_token_data(std::move(content_cell));
    if (r_content.is_error()) {
      return {r_content.move_as_error_prefix("Failed to parse jetton content from the cell: "), std::move(session)};
    }
    auto [content_onchain, content] = r_content.move_as_ok();
    data->content_onchain_ = content_onchain;
    data->content_ = std::move(content);
  }

  if (skip_verification) {
    return {std::move(data), std::move(session)};
  }

  // verification
  if (data->collection_address_.empty()) {
    data->is_validated_ = true;
    return {std::move(data), std::move(session)};
  }
  auto [r_parent_smc_info, new_session_3] = loadContract(data->collection_address_, seqno, archival, session);
  session = std::move(new_session_3);
  if (!r_parent_smc_info.is_ok()) {
    return {r_parent_smc_info.move_as_error(), session};
  }
  auto parent_smc_info = r_parent_smc_info.move_as_ok();

  // verify with master
  auto request_2 = multiclient::RequestFunction<tonlib_api::smc_runGetMethod>{
    .parameters = {.mode=multiclient::RequestMode::Single, .archival = archival},
    .request_creator = [id_ = parent_smc_info->id_, index_ = data->index_] {
      std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>> stack;
      auto tonlib_slice = tonlib_api::make_object<tonlib_api::tvm_numberDecimal>(index_);
      auto entry = tonlib_api::make_object<tonlib_api::tvm_stackEntryNumber>(std::move(tonlib_slice));
      stack.push_back(std::move(entry));
      return tonlib_api::make_object<tonlib_api::smc_runGetMethod>(
        id_,
        tonlib_api::make_object<tonlib_api::smc_methodIdName>("get_nft_address_by_index"),
        std::move(stack));
    },
    .session = std::move(session),
  };
  auto [res_2, new_session_4] = send_request_function(std::move(request_2));
  session = std::move(new_session_4);
  if (!res_2.is_ok()) {
    auto error = res_2.move_as_error();
    return {std::move(error), std::move(session)};
  }
  auto result_2 = res_2.move_as_ok();
  if (result_2->exit_code_ != 0) {
    LOG(ERROR) << "Exit code " << result_2->exit_code_ << " != 0";
    return {nullptr, std::move(session)};
  }
  auto& entry = static_cast<tonlib_api::tvm_stackEntryCell&>(*(result_2->stack_[0]));

  auto r_address_from_collection = tvm::address_from_tvm_stack_entry(result_2->stack_[0]);
  if (r_address_from_collection.is_error()) {
    return {r_address_from_collection.move_as_error(), std::move(session)};
  }
  auto address_from_collection = r_address_from_collection.move_as_ok();

  // check addresses
  auto r_address_std = block::StdAddress::parse(address);
  if (r_address_std.is_error()) {
    return {r_address_std.move_as_error(), std::move(session)};
  }
  auto address_std = r_address_std.move_as_ok();

  auto r_address_from_collection_std = block::StdAddress::parse(address_from_collection);
  if (r_address_from_collection_std.is_error()) {
    return {r_address_from_collection_std.move_as_error(), std::move(session)};
  }
  auto address_from_collection_std = r_address_from_collection_std.move_as_ok();

  LOG(ERROR) << "address: " << address_std << " expected: " << address_from_collection_std;
  if (address_from_collection_std != address_std) {
    return {td::Status::Error(409, "Verification on collection failed"), std::move(session)};
  }
  data->is_validated_ = true;

  // content_
  auto request_3 = multiclient::RequestFunction<tonlib_api::smc_runGetMethod>{
    .parameters = {.mode=multiclient::RequestMode::Single, .archival = archival},
    .request_creator = [id_ = parent_smc_info->id_, index_ = data->index_, ind_content = std::move(ind_content_cell_data)] {
      std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>> stack;
      auto tonlib_slice = tonlib_api::make_object<tonlib_api::tvm_numberDecimal>(index_);
      auto entry = tonlib_api::make_object<tonlib_api::tvm_stackEntryNumber>(std::move(tonlib_slice));
      stack.push_back(std::move(entry));

      auto tonlib_slice_2 = tonlib_api::make_object<tonlib_api::tvm_cell>(std::move(ind_content));
      auto entry_2 = tonlib_api::make_object<tonlib_api::tvm_stackEntryCell>(std::move(tonlib_slice_2));
      stack.push_back(std::move(entry_2));
      return tonlib_api::make_object<tonlib_api::smc_runGetMethod>(
        id_,
        tonlib_api::make_object<tonlib_api::smc_methodIdName>("get_nft_content"),
        std::move(stack));
    },
    .session = std::move(session),
  };
  auto [res_3, new_session_5] = send_request_function(std::move(request_3));
  session = std::move(new_session_5);
  if (!res_3.is_ok()) {
    auto error = res_3.move_as_error();
    return {std::move(error), std::move(session)};
  }
  auto result_3 = res_3.move_as_ok();
  if (result_3->exit_code_ != 0) {
    LOG(ERROR) << "Exit code " << result_3->exit_code_ << " != 0";
    return {nullptr, std::move(session)};
  }
  auto& content_data_str = static_cast<tonlib_api::tvm_stackEntryCell&>(*(result_3->stack_[0])).cell_->bytes_;

  auto r_content_cell = vm::std_boc_deserialize(content_data_str, true, true);
  if (r_content_cell.is_error()) {
    return {r_content_cell.move_as_error_prefix("Failed to parse jetton content cell: "), std::move(session)};
  }
  auto content_cell = r_content_cell.move_as_ok();

  auto r_content = tvm::parse_token_data(std::move(content_cell));
  if (r_content.is_error()) {
    return {r_content.move_as_error_prefix("Failed to parse jetton content from the cell: "), std::move(session)};
  }
  auto [content_onchain, content] = r_content.move_as_ok();
  data->content_onchain_ = content_onchain;
  data->content_ = std::move(content);

  // dns_entry_
  // TODO: implement dns entry parsing

  return {std::move(data), std::move(session)};
}
}  // namespace ton_http::core
