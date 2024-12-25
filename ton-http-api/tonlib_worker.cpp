#include "tonlib_worker.h"

#include "userver/formats/json.hpp"

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
td::Result<tonlib_api::blocks_getMasterchainInfo::ReturnType> TonlibWorker::getMasterchainInfo() const {
  auto request = multiclient::Request<tonlib_api::blocks_getMasterchainInfo>{
      .parameters = {.mode = multiclient::RequestMode::Multiple, .clients_number = 1},
      .request_creator = [] { return tonlib_api::blocks_getMasterchainInfo(); },
  };
  auto result = send_request(std::move(request), true);
  return std::move(result);
}
td::Result<tonlib_api::blocks_getMasterchainBlockSignatures::ReturnType> TonlibWorker::getMasterchainBlockSignatures(ton::BlockSeqno seqno) const {
  auto request = multiclient::Request<tonlib_api::blocks_getMasterchainBlockSignatures>{
    .parameters = {.mode = multiclient::RequestMode::Multiple, .clients_number = 1},
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
        .parameters = {.mode = multiclient::RequestMode::Multiple, .clients_number = 1},
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
        .parameters = {.mode = multiclient::RequestMode::Multiple, .clients_number = 1},
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
      .parameters = {.mode = multiclient::RequestMode::Multiple, .clients_number = 1},
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
      .parameters = {.mode = multiclient::RequestMode::Multiple, .clients_number = 1},
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
td::Result<tonlib_api::blocks_lookupBlock::ReturnType> TonlibWorker::lookupBlock(const ton::WorkchainId& workchain, const ton::ShardId& shard, const std::optional<ton::BlockSeqno>& seqno,
    const std::optional<ton::LogicalTime>& lt, const std::optional<ton::UnixTime>& unixtime) const {
  if (!(seqno.has_value() || lt.has_value() && unixtime.has_value())) {
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
  auto request = multiclient::Request<tonlib_api::blocks_lookupBlock>{
    .parameters = {.mode = multiclient::RequestMode::Multiple, .clients_number = 1},
    .request_creator = [lookupMode, workchain, shard, seqno, lt, unixtime] {
      return tonlib_api::blocks_lookupBlock(
        lookupMode,
        tonlib_api::make_object<tonlib_api::ton_blockId>(workchain, shard, value_or_default(seqno, 0)),
        value_or_default(lt, 0),
        value_or_default(unixtime, 0)
      );
    },
  };
  auto result = send_request(std::move(request), true);
  return std::move(result);
};
}  // namespace ton_http::core
