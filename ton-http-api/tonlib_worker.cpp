#include "tonlib_worker.h"

#include "userver/formats/json.hpp"
#include "utils.hpp"

namespace ton_http::core {

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

td::Result<ConsensusBlockResult> TonlibWorker::getConsensusBlock() const {
  auto res = tonlib_.get_consensus_block();
  if (res.is_error()) {
    return res.move_as_error();
  }
  return ConsensusBlockResult{ res.move_as_ok(), std::time(nullptr) };
}
td::Result<DetectAddressResult> TonlibWorker::detectAddress(const std::string& address) const {
  auto r_std_address = block::StdAddress::parse(address);
  if (r_std_address.is_error()) {
    return r_std_address.move_as_error();
  }
  const auto std_address = r_std_address.move_as_ok();
  std::string given_type = "raw_form";
  if (address.length() == 48) {
    given_type = std::string("friendly_") + (std_address.bounceable ? "bounceable" : "non_bounceable");
  }
  DetectAddressResult result{std_address, given_type};
  return std::move(result);
}
td::Result<std::string> TonlibWorker::packAddress(const std::string& address) const {
  auto r_std_address = block::StdAddress::parse(address);
  if (r_std_address.is_error()) {
    return r_std_address.move_as_error();
  }
  const auto std_address = r_std_address.move_as_ok();
  return std::move(std_address.rserialize(true));
}

td::Result<std::string> TonlibWorker::unpackAddress(const std::string& address) const {
  block::StdAddress std_address;
  if (std_address.rdeserialize(address)) {
    const DetectAddressResult result{std_address, "unknown"};
    return result.to_raw_form(true);
  }
}
td::Result<DetectHashResult> TonlibWorker::detectHash(const std::string& hash) const {
  auto raw_hash = utils::stringToHash(hash);
  if (!raw_hash.has_value()) {
    return td::Status::Error("Invalid hash");
  }
  DetectHashResult result{raw_hash.value()};
  return std::move(result);
}
td::Result<tonlib_api::blocks_getMasterchainInfo::ReturnType> TonlibWorker::getMasterchainInfo() const {
  auto request = multiclient::Request<tonlib_api::blocks_getMasterchainInfo>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator = [] { return tonlib_api::blocks_getMasterchainInfo(); },
  };
  auto result = send_request(std::move(request), true);
  return std::move(result);
}
td::Result<tonlib_api::blocks_getMasterchainBlockSignatures::ReturnType> TonlibWorker::getMasterchainBlockSignatures(ton::BlockSeqno seqno) const {
  auto request = multiclient::Request<tonlib_api::blocks_getMasterchainBlockSignatures>{
    .parameters = {.mode = multiclient::RequestMode::Single},
    .request_creator = [seqno] { return tonlib_api::blocks_getMasterchainBlockSignatures(seqno); },
  };
  auto result = send_request(std::move(request), true);
  return std::move(result);
}
td::Result<tonlib_api::raw_getAccountState::ReturnType> TonlibWorker::getAddressInformation(
    std::string address, std::optional<std::int32_t> seqno
) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> with_block;
  if (seqno.has_value()) {
    auto res = lookupBlock(ton::masterchainId, ton::shardIdAll, seqno.value());
    if (!res.is_ok()) {
      return res.move_as_error();
    }
    with_block = res.move_as_ok();
  }
  if (!with_block) {
    auto request = multiclient::RequestFunction<tonlib_api::raw_getAccountState>{
        .parameters = {.mode = multiclient::RequestMode::Single},
        .request_creator =
            [address_ = address] {
              return tonlib_api::make_object<tonlib_api::raw_getAccountState>(
                  tonlib_api::make_object<tonlib_api::accountAddress>(address_)
              );
            }
    };
    auto result = send_request_function(std::move(request), true);
    return std::move(result);
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
            }
    };
    auto result = send_request_function(std::move(request), true);
    if (result.is_error()) {
      return result.move_as_error();
    }
    return ton::move_tl_object_as<tonlib_api::raw_fullAccountState>(result.move_as_ok());
  }
}
td::Result<tonlib_api::getAccountState::ReturnType> TonlibWorker::getExtendedAddressInformation(
    std::string address, std::optional<std::int32_t> seqno
) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> with_block;
  if (seqno.has_value()) {
    auto res = lookupBlock(ton::masterchainId, ton::shardIdAll, seqno.value());
    if (!res.is_ok()) {
      return res.move_as_error();
    }
    with_block = res.move_as_ok();
  }
  if (!with_block) {
    auto request = multiclient::RequestFunction<tonlib_api::getAccountState>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [address_ = address] {
            return tonlib_api::make_object<tonlib_api::getAccountState>(
                tonlib_api::make_object<tonlib_api::accountAddress>(address_)
            );
      }
    };
    auto result = send_request_function(std::move(request), true);
    return std::move(result);
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
                tonlib_api::make_object<tonlib_api::getAccountState>(
                    tonlib_api::make_object<tonlib_api::accountAddress>(address_)
                )
            );
      }
    };
    auto result = send_request_function(std::move(request), true);
    if (result.is_error()) {
      return result.move_as_error();
    }
    return ton::move_tl_object_as<tonlib_api::fullAccountState>(result.move_as_ok());
  }
}
td::Result<tonlib_api::blocks_lookupBlock::ReturnType> TonlibWorker::lookupBlock(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const std::optional<ton::BlockSeqno>& seqno,
    const std::optional<ton::LogicalTime>& lt,
    const std::optional<ton::UnixTime>& unixtime
) const {
  if (!(seqno.has_value() || lt.has_value() || unixtime.has_value())) {
    return td::Status::Error(416, "one of seqno, lt, unixtime should be specified");
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
  };
  auto result = send_request_function(std::move(request), true);
  return std::move(result);
}
td::Result<tonlib_api::blocks_getShardBlockProof::ReturnType> TonlibWorker::getShardBlockProof(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const std::optional<ton::BlockSeqno>& from_seqno
) const {
  std::int32_t mode = 0;
  auto r_blk_id = lookupBlock(workchain, shard, seqno);
  if (r_blk_id.is_error()) {
    return r_blk_id.move_as_error();
  }
  auto blk_id = r_blk_id.move_as_ok();

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
          }
  };

  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> from_blk_id = nullptr;
  if (from_seqno.has_value()) {
    mode = 1;
    auto r_from_blk_id = lookupBlock(ton::masterchainId, ton::shardIdAll, from_seqno.value());
    if (r_from_blk_id.is_error()) {
      return r_from_blk_id.move_as_error_prefix("lookup from block error: ");
    }
    from_blk_id = r_from_blk_id.move_as_ok();
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
  auto result = send_request_function(std::move(request), true);
  return std::move(result);
}
td::Result<tonlib_api::blocks_getShards::ReturnType> TonlibWorker::getShards(
    std::optional<ton::BlockSeqno> mc_seqno, std::optional<ton::LogicalTime> lt, std::optional<ton::UnixTime> unixtime
) const {
  auto r_blk_id = lookupBlock(ton::masterchainId, ton::shardIdAll, mc_seqno, lt, unixtime);
  if (r_blk_id.is_error()) {
    return r_blk_id.move_as_error();
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
          }
  };
  auto result = send_request_function(std::move(request), true);
  return std::move(result);
}
td::Result<tonlib_api::blocks_getBlockHeader::ReturnType> TonlibWorker::getBlockHeader(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const std::string& root_hash,
    const std::string& file_hash
) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> blk_id = nullptr;
  if (!root_hash.empty() && !file_hash.empty()) {
    blk_id = tonlib_api::make_object<tonlib_api::ton_blockIdExt>(workchain, shard, seqno, root_hash, file_hash);
  } else {
    auto r_blk_id = lookupBlock(workchain, shard, seqno);
    if (r_blk_id.is_error()) {
      return r_blk_id.move_as_error();
    }
    blk_id = r_blk_id.move_as_ok();
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
          }
  };
  auto result = send_request_function(std::move(request), true);
  return std::move(result);
}
td::Result<tonlib_api::blocks_getOutMsgQueueSizes::ReturnType> TonlibWorker::getOutMsgQueueSizes() const {
  auto request = multiclient::RequestFunction<tonlib_api::blocks_getOutMsgQueueSizes>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [] {
            return tonlib_api::make_object<tonlib_api::blocks_getOutMsgQueueSizes>();
          }
  };
  auto result = send_request_function(std::move(request), true);
  return std::move(result);
}
td::Result<tonlib_api::getConfigParam::ReturnType> TonlibWorker::getConfigParam(
    const std::int32_t& param, std::optional<ton::BlockSeqno> seqno
) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> with_block;
  if (seqno.has_value()) {
    auto res = lookupBlock(ton::masterchainId, ton::shardIdAll, seqno.value());
    if (!res.is_ok()) {
      return res.move_as_error();
    }
    with_block = res.move_as_ok();
  }
  if (!with_block) {
    auto request = multiclient::RequestFunction<tonlib_api::getConfigParam>{
        .parameters = {.mode = multiclient::RequestMode::Single},
        .request_creator =
            [param_ = param] {
              return tonlib_api::make_object<tonlib_api::getConfigParam>(param_, 0);
            }
    };
    auto result = send_request_function(std::move(request), true);
    return std::move(result);
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
            }
    };
    auto result = send_request_function(std::move(request), true);
    if (result.is_error()) {
      return result.move_as_error();
    }
    return ton::move_tl_object_as<tonlib_api::configInfo>(result.move_as_ok());
  }
}
td::Result<tonlib_api::getConfigParam::ReturnType> TonlibWorker::getConfigAll(std::optional<ton::BlockSeqno> seqno) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> with_block;
  if (seqno.has_value()) {
    auto res = lookupBlock(ton::masterchainId, ton::shardIdAll, seqno.value());
    if (!res.is_ok()) {
      return res.move_as_error();
    }
    with_block = res.move_as_ok();
  }
  if (!with_block) {
    auto request = multiclient::RequestFunction<tonlib_api::getConfigAll>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [] {
            return tonlib_api::make_object<tonlib_api::getConfigAll>(0);
      }
    };
    auto result = send_request_function(std::move(request), true);
    return std::move(result);
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
      }
    };
    auto result = send_request_function(std::move(request), true);
    if (result.is_error()) {
      return result.move_as_error();
    }
    return ton::move_tl_object_as<tonlib_api::configInfo>(result.move_as_ok());
  }
}
td::Result<tonlib_api::blocks_getTransactions::ReturnType> TonlibWorker::raw_getBlockTransactions(
    const tonlib_api::object_ptr<tonlib_api::ton_blockIdExt>& blk_id,
    size_t count,
    tonlib_api::object_ptr<tonlib_api::blocks_accountTransactionId>&& after,
    std::optional<bool> archival
) const {
  auto request = multiclient::RequestFunction<tonlib_api::blocks_getTransactions>{
      .parameters = {.mode = multiclient::RequestMode::Single},
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
          }
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
  if (archival.has_value()) {
    request.parameters.archival = archival.value();
  }
  auto result = send_request_function(std::move(request), !archival.has_value());
  return std::move(result);
}
td::Result<tonlib_api::blocks_getTransactionsExt::ReturnType> TonlibWorker::raw_getBlockTransactionsExt(
    const tonlib_api::object_ptr<tonlib_api::ton_blockIdExt>& blk_id,
    size_t count,
    tonlib_api::object_ptr<tonlib_api::blocks_accountTransactionId>&& after,
    std::optional<bool> archival
) const {
  auto request = multiclient::RequestFunction<tonlib_api::blocks_getTransactionsExt>{
      .parameters = {.mode = multiclient::RequestMode::Single},
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
          }
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
  if (archival.has_value()) {
    request.parameters.archival = archival.value();
  }
  auto result = send_request_function(std::move(request), !archival.has_value());
  return std::move(result);
}
td::Result<tonlib_api::raw_getTransactions::ReturnType> TonlibWorker::raw_getTransactions(
    const std::string& account_address,
    const ton::LogicalTime& from_transaction_lt,
    const std::string& from_transaction_hash,
    const std::optional<bool> archival
) const {
  auto request = multiclient::RequestFunction<tonlib_api::raw_getTransactions>{
      .parameters = {.mode = multiclient::RequestMode::Single},
      .request_creator =
          [a = account_address, fl = from_transaction_lt, fh = from_transaction_hash] {
            return tonlib_api::make_object<tonlib_api::raw_getTransactions>(
                nullptr,
                tonlib_api::make_object<tonlib_api::accountAddress>(a),
                tonlib_api::make_object<tonlib_api::internal_transactionId>(fl, fh)
            );
          }
  };
  if (archival.has_value()) {
    request.parameters.archival = archival.value();
  }
  auto result = send_request_function(std::move(request), !archival.has_value());
  return std::move(result);
}
td::Result<tonlib_api::raw_getTransactionsV2::ReturnType> TonlibWorker::raw_getTransactionsV2(
    const std::string& account_address,
    const ton::LogicalTime& from_transaction_lt,
    const std::string& from_transaction_hash,
    const size_t count,
    const bool try_decode_messages,
    const std::optional<bool> archival
) const {
  auto request = multiclient::RequestFunction<tonlib_api::raw_getTransactionsV2>{
    .parameters = {.mode = multiclient::RequestMode::Single},
    .request_creator = [a = account_address, fl = from_transaction_lt, fh = from_transaction_hash, c = count, dm = try_decode_messages] {
      return tonlib_api::make_object<tonlib_api::raw_getTransactionsV2>(
        nullptr,
        tonlib_api::make_object<tonlib_api::accountAddress>(a),
        tonlib_api::make_object<tonlib_api::internal_transactionId>(fl, fh),
        c, dm
        );
    }};
  if (archival.has_value()) {
    request.parameters.archival = archival.value();
  }
  auto result = send_request_function(std::move(request), !archival.has_value());
  return std::move(result);
}
td::Result<tonlib_api::blocks_getTransactions::ReturnType> TonlibWorker::getBlockTransactions(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const size_t count,
    const std::string& root_hash,
    const std::string& file_hash,
    const std::optional<ton::LogicalTime>& after_lt,
    const std::string after_hash,
    std::optional<bool> archival
) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> blk_id = nullptr;
  if (!root_hash.empty() && !file_hash.empty()) {
    blk_id = tonlib_api::make_object<tonlib_api::ton_blockIdExt>(workchain, shard, seqno, root_hash, file_hash);
  } else {
    auto r_blk_id = lookupBlock(workchain, shard, seqno);
    if (r_blk_id.is_error()) {
      return r_blk_id.move_as_error();
    }
    blk_id = r_blk_id.move_as_ok();
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
    auto result = raw_getBlockTransactions(blk_id, chunk_size, std::move(after), archival);
    if (result.is_error()) {
      return result.move_as_error();
    }
    auto local = result.move_as_ok();

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

  return std::move(txs);
}
td::Result<tonlib_api::blocks_getTransactionsExt::ReturnType> TonlibWorker::getBlockTransactionsExt(
    const ton::WorkchainId& workchain,
    const ton::ShardId& shard,
    const ton::BlockSeqno& seqno,
    const size_t count,
    const std::string& root_hash,
    const std::string& file_hash,
    const std::optional<ton::LogicalTime>& after_lt,
    const std::string after_hash,
    std::optional<bool> archival
) const {
  tonlib_api::object_ptr<tonlib_api::ton_blockIdExt> blk_id = nullptr;
  if (!root_hash.empty() && !file_hash.empty()) {
    blk_id = tonlib_api::make_object<tonlib_api::ton_blockIdExt>(workchain, shard, seqno, root_hash, file_hash);
  } else {
    auto r_blk_id = lookupBlock(workchain, shard, seqno);
    if (r_blk_id.is_error()) {
      return r_blk_id.move_as_error();
    }
    blk_id = r_blk_id.move_as_ok();
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
    auto result = raw_getBlockTransactionsExt(blk_id, chunk_size, std::move(after), archival);
    if (result.is_error()) {
      return result.move_as_error();
    }
    auto local = result.move_as_ok();

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
  return std::move(txs);
}
td::Result<tonlib_api::raw_getTransactionsV2::ReturnType> TonlibWorker::getTransactions(
    const std::string& account_address,
    std::optional<ton::LogicalTime> from_transaction_lt,
    std::string from_transaction_hash,
    ton::LogicalTime to_transaction_lt,
    size_t count,
    size_t chunk_size,
    bool try_decode_messages,
    std::optional<bool> archival
) const {
  if (!(from_transaction_lt.has_value() && !from_transaction_hash.empty())) {
    auto r_account_state_ = getAddressInformation(account_address);
    if (r_account_state_.is_error()) {
      return r_account_state_.move_as_error();
    }
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
    auto r_local = raw_getTransactionsV2(
        account_address, current_lt, current_hash, local_chunk_size, try_decode_messages, archival
    );
    if (r_local.is_error()) {
      return r_local.move_as_error();
    }
    auto local = r_local.move_as_ok();

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
  return std::move(txs);
}
td::Result<tonlib_api::raw_getTransactionsV2::ReturnType> TonlibWorker::tryLocateTransactionByIncomingMessage(
    const std::string& source, const std::string& destination, ton::LogicalTime created_lt
) const {
  auto r_src_addr = block::StdAddress::parse(source);
  if (r_src_addr.is_error()) {
    return r_src_addr.move_as_error_prefix("failed to parse source: ");
  }
  auto src = r_src_addr.move_as_ok();

  auto r_dest_addr = block::StdAddress::parse(destination);
  if (r_dest_addr.is_error()) {
    return r_dest_addr.move_as_error_prefix("failed to parse destination: ");
  }
  auto dest = r_dest_addr.move_as_ok();

  auto workchain = dest.workchain;
  auto r_shards = getShards(std::nullopt, created_lt, std::nullopt);
  if (r_shards.is_error()) {
    return r_shards.move_as_error_prefix("failed to get shards at create_lt: ");
  }
  auto shards = r_shards.move_as_ok();
  for (auto& shard : shards->shards_) {
    auto shard_id = shard->shard_;
    for (auto i = 0; i < 3; ++i) {
      auto lt = created_lt + 1000000 * i;
      auto r_block = lookupBlock(workchain, shard_id, std::nullopt, lt, std::nullopt);
      if (r_block.is_error()) {
        td::StringBuilder sb;
        sb << "failed to lookup block with lt " << lt << ": ";
        return r_block.move_as_error_prefix(sb.as_cslice().str());
      }
      auto block = r_block.move_as_ok();

      constexpr size_t tx_count = 40;
      auto r_txs = getBlockTransactions(
          block->workchain_, block->shard_, block->seqno_, tx_count, block->root_hash_, block->file_hash_
      );
      if (r_txs.is_error()) {
        td::StringBuilder sb;
        sb << "failed to get transactions for block (" << block->workchain_ << ", " << block->shard_ << ", "
           << block->seqno_ << "): ";
        return r_txs.move_as_error_prefix(sb.as_cslice().str());
      }
      auto blk_txs = r_txs.move_as_ok();

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
        auto r_candidate_txs =
            getTransactions(destination, candidate->lt_, candidate->hash_, std::max(tx_found, min_tx_found));
        if (r_txs.is_error()) {
          return r_txs.move_as_error_prefix("failed to get candidate transactions: ");
        }
        auto candidate_txs = r_candidate_txs.move_as_ok();
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
              return tonlib_api::make_object<tonlib_api::raw_transactions>(std::move(tx_vec), std::move(prev_tx));
            }
          }
        }
      }
    }
  }
  return td::Status::Error(404, "transaction was not found");
}
td::Result<tonlib_api::raw_getTransactionsV2::ReturnType> TonlibWorker::tryLocateTransactionByOutgoingMessage(
    const std::string& source, const std::string& destination, ton::LogicalTime created_lt
) const {
  auto r_src_addr = block::StdAddress::parse(source);
  if (r_src_addr.is_error()) {
    return r_src_addr.move_as_error_prefix("failed to parse source: ");
  }
  auto src = r_src_addr.move_as_ok();

  auto r_dest_addr = block::StdAddress::parse(destination);
  if (r_dest_addr.is_error()) {
    return r_dest_addr.move_as_error_prefix("failed to parse destination: ");
  }
  auto dest = r_dest_addr.move_as_ok();

  auto workchain = src.workchain;
  auto r_shards = getShards(std::nullopt, created_lt, std::nullopt);
  if (r_shards.is_error()) {
    return r_shards.move_as_error_prefix("failed to get shards at create_lt: ");
  }
  auto shards = r_shards.move_as_ok();
  for (auto& shard : shards->shards_) {
    auto shard_id = shard->shard_;
    auto r_block = lookupBlock(workchain, shard_id, std::nullopt, created_lt, std::nullopt);
    if (r_block.is_error()) {
      td::StringBuilder sb;
      sb << "failed to lookup block with lt " << created_lt << ": ";
      return r_block.move_as_error_prefix(sb.as_cslice().str());
    }
    auto block = r_block.move_as_ok();

    constexpr size_t tx_count = 40;
    auto r_txs = getBlockTransactions(
        block->workchain_, block->shard_, block->seqno_, tx_count, block->root_hash_, block->file_hash_
    );
    if (r_txs.is_error()) {
      td::StringBuilder sb;
      sb << "failed to get transactions for block (" << block->workchain_ << ", " << block->shard_ << ", "
         << block->seqno_ << "): ";
      return r_txs.move_as_error_prefix(sb.as_cslice().str());
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
      auto r_candidate_txs =
          getTransactions(source, candidate->lt_, candidate->hash_, std::max(tx_found, min_tx_found));
      if (r_txs.is_error()) {
        return r_txs.move_as_error_prefix("failed to get candidate transactions: ");
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
              return tonlib_api::make_object<tonlib_api::raw_transactions>(std::move(tx_vec), std::move(prev_tx));
            }
          }
        }
      }
    }
  }
  return td::Status::Error(404, "transaction was not found");
}
td::Result<tonlib_api::raw_sendMessage::ReturnType> TonlibWorker::raw_sendMessage(const std::string& boc) const {
  auto r_boc = td::base64_decode(boc);
  if (r_boc.is_error()) {
    return r_boc.move_as_error();
  }
  auto boc_bytes = r_boc.move_as_ok();
  auto request = multiclient::RequestFunction<tonlib_api::raw_sendMessage>{
    .parameters = {.mode=multiclient::RequestMode::Multiple, .clients_number = 5},
    .request_creator = [boc_bytes]() {
      return tonlib_api::make_object<tonlib_api::raw_sendMessage>(boc_bytes);
    }
  };
  auto result = send_request_function(std::move(request), false);
  return std::move(result);
}
td::Result<tonlib_api::raw_sendMessageReturnHash::ReturnType> TonlibWorker::raw_sendMessageReturnHash(
    const std::string& boc
) const {
  auto r_boc = td::base64_decode(boc);
  if (r_boc.is_error()) {
    return r_boc.move_as_error();
  }
  auto boc_bytes = r_boc.move_as_ok();
  auto request = multiclient::RequestFunction<tonlib_api::raw_sendMessageReturnHash>{
    .parameters = {.mode=multiclient::RequestMode::Multiple, .clients_number = 5},
    .request_creator = [boc_bytes]() {
      return tonlib_api::make_object<tonlib_api::raw_sendMessageReturnHash>(boc_bytes);
    }
  };
  auto result = send_request_function(std::move(request), false);
  return std::move(result);
}
}  // namespace ton_http::core
