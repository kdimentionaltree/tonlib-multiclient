#include "utils.hpp"

#include "auto/tl/tonlib_api.hpp"
#include "auto/tl/tonlib_api_json.h"
#include "block/block.h"
#include "tl/tl_json.h"

#include "common/refint.h"
#include "overlay/overlay-broadcast.hpp"
#include "td/utils/JsonBuilder.h"
#include "td/utils/Status.h"
#include "td/utils/logging.h"
#include "td/utils/overloaded.h"
#include "vm/boc.h"
#include "vm/cells/CellSlice.h"

#include "tokens-tlb.h"
#include "common/checksum.h"


using namespace ton;

std::string to_hex_string_with_prefix(td::RefInt256& val) {
  auto res = val->to_hex_string();
  if (res[0] == '-') {
    return "-0x" + res.substr(1, res.size() - 1);
  }
  return "0x" + res;
}


td::Result<userver::formats::json::Value>  ton_http::utils::render_tvm_stack(const std::string& stack_str) {
  using namespace userver::formats::json;

  auto result = ValueBuilder();

  auto stack_raw = FromString(stack_str);
  if (!stack_raw.IsArray()) {
    return td::Status::Error(422, "Stack should be array");
  }

  for (size_t idx = 0; idx < stack_raw.GetSize(); ++idx) {
    auto element_raw = stack_raw[idx];

    if (!element_raw.IsArray()) {
      std::stringstream ss;
      ss << "Stack element " << idx << " should be array";
      return td::Status::Error(422, ss.str());
    }
    if (element_raw.GetSize() != 2) {
      std::stringstream ss;
      ss << "Stack element " << idx << " should have 2 elements";
      return td::Status::Error(422, ss.str());
    }
    auto r_element_rendered = render_tvm_element(element_raw[0].As<std::string>(), element_raw[1]);
    if (r_element_rendered.is_error()) {
      std::stringstream ss;
      ss << "Failed to render element " << idx << ": ";
      return r_element_rendered.move_as_error_prefix(ss.str());
    }
    auto element_rendered = r_element_rendered.move_as_ok();
    result.PushBack(element_rendered);
  }

  return result.ExtractValue();
}


td::Result<std::int64_t> _parse_int(const userver::formats::json::Value& element) {
  try {
    auto value = element.As<int64_t>();
    return value;
  }
  catch (std::exception& ee) {
    std::stringstream ss;
    ss << "Failed to parse int: " << ee.what();
    return td::Status::Error(ss.str());
  }
}


td::Result<userver::formats::json::Value> ton_http::utils::render_tvm_element(
    const std::string& element_type, const userver::formats::json::Value& element
) {
  using namespace userver::formats::json;
  auto result = ValueBuilder();

  LOG(INFO) << "Element type: '" << element_type << "'";
  if (element_type == "num" || element_type == "number" || element_type == "int") {
    auto r_value = _parse_int(element);
    if (r_value.is_error()) {
      return r_value.move_as_error();
    }

    result["@type"] = "tvm.stackEntryNumber";
    result["number"]["@type"] = "tvm.numberDecimal";
    result["number"]["number"] = r_value.move_as_ok();
    return result.ExtractValue();
  }
  if (element_type == "cell") {
    auto element_str = element.As<std::string>();
    result["@type"] = "tvm.stackEntryCell";
    result["cell"]["@type"] = "tvm.Cell";
    result["cell"]["bytes"] = element_str;
    return result.ExtractValue();
  }

  std::stringstream ss;
  ss << "Unsupported stack entry type: " << element_type;
  return td::Status::Error(422, ss.str());
}


userver::formats::json::Value ton_http::utils::serialize_tvm_entry(tonlib_api::tvm_stackEntryCell& entry) {
  using namespace userver::formats::json;
  ValueBuilder builder;

  builder["bytes"] = td::base64_encode(entry.cell_->bytes_);

  auto r_cell = vm::std_boc_deserialize(entry.cell_->bytes_);
  if (r_cell.is_error()) {
    builder["object"] ["error"] = r_cell.move_as_error().to_string();
    return builder.ExtractValue();
  }
  auto cell = r_cell.move_as_ok();
  builder["object"] = serialize_cell(cell);

  return builder.ExtractValue();
}


userver::formats::json::Value ton_http::utils::serialize_tvm_entry(tonlib_api::tvm_stackEntrySlice& entry) {
  using namespace userver::formats::json;
  ValueBuilder builder;

  builder["bytes"] = td::base64_encode(entry.slice_->bytes_);
  auto r_cell = vm::std_boc_deserialize(entry.slice_->bytes_);
  if (r_cell.is_error()) {
    builder["object"] ["error"] = r_cell.move_as_error().to_string();
    return builder.ExtractValue();
  }
  auto cell = r_cell.move_as_ok();
  builder["object"] = serialize_cell(cell);

  return builder.ExtractValue();
}


std::string serialize_data_cell(td::Ref<vm::DataCell>& cell) {
  auto size = (cell->get_bits() + 7) / 8;
  std::string result;
  result.resize(size);
  std::memcpy(result.data(), cell->get_data(), size);
  return result;
}

userver::formats::json::Value ton_http::utils::serialize_cell(td::Ref<vm::Cell>& cell) {
  using namespace userver::formats::json;
  ValueBuilder builder;

  bool is_special = false;
  auto cs = vm::load_cell_slice_special(cell, is_special);
  auto r_ls = cell->load_cell();
  if (r_ls.is_error()) {
    builder["error"] = r_ls.move_as_error().to_string();
    return builder.ExtractValue();
  }
  auto ls = r_ls.move_as_ok();
  builder["data"]["b64"] = td::base64_encode(serialize_data_cell(ls.data_cell));
  builder["data"]["len"] = ls.data_cell->get_bits();
  builder["refs"] = FromString("[]");
  builder["special"] = ls.data_cell->is_special();

  while (cs.have_refs(1)) {
    cs.advance(1);
    auto loc = cs.fetch_ref();
    builder["refs"].PushBack(serialize_cell(loc));
  }
  return builder.ExtractValue();
}

td::Result<std::string> ton_http::utils::address_from_cell(std::string data) {
  auto r_cell = vm::std_boc_deserialize(std::move(data), true, false);
  if (r_cell.is_error()) {
    return td::Status::Error(500, r_cell.move_as_error().to_string());
  }
  auto cell = r_cell.move_as_ok();
  auto cs = vm::load_cell_slice(cell);
  switch ((unsigned)cs.prefetch_ulong(2)) {
    case 0:
      return "";
    case 1:
      return td::Status::Error(500, "addr_ext is not supported");
    case 2: {
      cs.advance(2);
      if (cs.prefetch_ulong(1)) {
        return td::Status::Error(500, "anycast is not supported");
      }
      cs.advance(1);

      auto workchain_id = cs.fetch_long(8);
      auto addr = cs.fetch_bits(256);
      auto address = block::StdAddress{static_cast<ton::WorkchainId>(workchain_id), addr.bits(), true, false};
      return address.rserialize(true);
    }
    case 3:
      return td::Status::Error(500, "addr_var is not supported");
  }
  UNREACHABLE();
}
td::Result<std::string> ton_http::utils::address_from_tvm_stack_entry(tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>& entry) {
  std::string data;
  if (entry->get_id() == tonlib_api::tvm_stackEntryCell::ID) {
    auto& entry_cell = static_cast<tonlib_api::tvm_stackEntryCell&>(*entry);
    data = entry_cell.cell_->bytes_;
  } else if (entry->get_id() == tonlib_api::tvm_stackEntrySlice::ID) {
    auto& entry_slice = static_cast<tonlib_api::tvm_stackEntrySlice&>(*entry);
    data = entry_slice.slice_->bytes_;
  }

  return address_from_cell(data);
}


td::Result<std::string> ton_http::utils::number_from_tvm_stack_entry(tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>& entry) {
  if (entry->get_id() != tonlib_api::tvm_stackEntryNumber::ID) {
    return td::Status::Error(500, "stackEntryNumber expected");
  }
   return static_cast<const tonlib_api::tvm_stackEntryNumber&>(*entry).number_->number_;
}


td::Result<std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>> ton_http::utils::parse_stack(const std::vector<std::string>& stack_vector) {
  if (stack_vector.empty()) {
    return std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>{};
  }
  std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>> stack;
  stack.reserve(stack_vector.size());
  for (auto &stack_item_const : stack_vector) {
    std::string stack_item_str = stack_item_const;
    TRY_RESULT(value, td::json_decode(td::MutableSlice(stack_item_str)));

    if (value.type() != td::JsonValue::Type::Array) {
      return td::Status::Error(422, "Invalid stack format: array expected");
    }
    auto &array = value.get_array();
    if (array.size() != 2) {
      return td::Status::Error(422, "Invalid stack entry format: array of exact 2 elemets expected");
    }
    auto tp = array[0].get_string().str();
    auto &val = array[1];
    if (tp == "int" || tp == "integer" || tp == "num" || tp == "number") {
      std::string int_value;
      if (val.type() == td::JsonValue::Type::Number) {
        int_value = val.get_number().str();
      } else if (val.type() == td::JsonValue::Type::String) {
        auto parsed_int = td::string_to_int256(val.get_string().str());
        if (parsed_int.is_null()) {
          td::StringBuilder sb;
          sb << "Invalid stack entry format: invalid integer value " << val.get_string();
          return td::Status::Error(422, sb.as_cslice());
        }
        int_value = parsed_int->to_dec_string();
      }
      auto entry = tonlib_api::make_object<tonlib_api::tvm_stackEntryNumber>(
        tonlib_api::make_object<tonlib_api::tvm_numberDecimal>(int_value)
      );
      stack.push_back(std::move(entry));
    } else if (tp == "tvm.Cell" || tp == "tvm.Slice") {
      if (val.type() != td::JsonValue::Type::String) {
        return td::Status::Error(422, "Invalid stack entry format: base64 encoded string expected");
      }
      auto r_bytes = td::base64_decode(val.get_string());
      if (r_bytes.is_error()) {
        td::StringBuilder sb;
        sb << "Invalid stack entry format: invalid base64 encoded string: " << r_bytes.move_as_error();
        return td::Status::Error(422, sb.as_cslice());
      }
      auto bytes = r_bytes.move_as_ok();
      if (tp == "tvm.Cell") {
        auto entry = tonlib_api::make_object<tonlib_api::tvm_stackEntryCell>(
          tonlib_api::make_object<tonlib_api::tvm_cell>(bytes)
        );
        stack.push_back(std::move(entry));
      } else if (tp == "tvm.Slice") {
        auto entry = tonlib_api::make_object<tonlib_api::tvm_stackEntrySlice>(
          tonlib_api::make_object<tonlib_api::tvm_slice>(bytes)
        );
        stack.push_back(std::move(entry));
      }
    } else {
      td::StringBuilder sb;
      sb << "Invalid stack entry format: invalid type " << tp;
      return td::Status::Error(422, sb.as_cslice());
    }
  }
  return std::move(stack);
}

userver::formats::json::Value ton_http::utils::serialize_tvm_stack(
    std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>& tvm_stack
) {
  using namespace userver::formats::json;
  ValueBuilder builder;
  for (auto & entry : tvm_stack) {
    ValueBuilder entry_builder;
    tonlib_api::downcast_call(*entry, td::overloaded(
      [&](tonlib_api::tvm_stackEntryCell& val) {
        entry_builder.PushBack("cell");
        auto res = serialize_tvm_entry(val);
        entry_builder.PushBack(res);
      },
      [&](tonlib_api::tvm_stackEntrySlice& val) {
        entry_builder.PushBack("cell");
        auto res = serialize_tvm_entry(val);
        entry_builder.PushBack(res);
      },
      [&](tonlib_api::tvm_stackEntryNumber& val) {
        entry_builder.PushBack("num");
        auto res = td::string_to_int256(val.number_->number_);
        entry_builder.PushBack(to_hex_string_with_prefix(res));
      },
      [&](tonlib_api::tvm_stackEntryTuple& val) {
        entry_builder.PushBack("tuple");
        entry_builder.PushBack(FromString(td::json_encode<td::string>(td::ToJson(val.tuple_))));
      },
      [&](tonlib_api::tvm_stackEntryList& val) {
        entry_builder.PushBack("list");
        entry_builder.PushBack(FromString(td::json_encode<td::string>(td::ToJson(val.list_))));
      },
      [&](tonlib_api::tvm_stackEntryUnsupported& val) {
        entry_builder.PushBack("unsupported");
        entry_builder.PushBack(FromString("{}"));
      }
      ));
    builder.PushBack(entry_builder.ExtractValue());
  }
  return builder.ExtractValue();
}

td::Result<std::string> ton_http::utils::parse_snake_data(td::Ref<vm::CellSlice> data) {
  size_t bsize = 1024 * 8;
  unsigned char buffer[bsize];
  td::BitPtr bw{buffer};

  while (true) {
    if (bw.offs + data->size() > bsize * 8) {
      break; // buffer overflow
    }
    bw.concat(data->prefetch_bits(data->size()));

    auto cell = data->prefetch_ref();
    if (cell.is_null()) {
      break;
    }
    data = vm::load_cell_slice_ref(cell);
  }

  if (!bw.byte_aligned()) {
    return td::Status::Error("Not byte aligned");
  }

  return std::string(buffer, bw.get_byte_ptr());
}

td::Result<std::string> ton_http::utils::parse_chunks_data(td::Ref<vm::CellSlice> data) {
  try {
    vm::Dictionary dict(data, 32);

    size_t bsize = 1024 * 8;
    unsigned char buffer[bsize];
    td::BitPtr bw{buffer};

    uint c = 0;
    while (dict.uint_key_exists(c)) {
      auto value = dict.lookup_ref(td::BitArray<32>(c));
      if (value.not_null()) {
        auto data = vm::load_cell_slice_ref(value);
        if (bw.offs + data->size() > bsize * 8) {
          break; // buffer overflow
        }

        bw.concat(data->prefetch_bits(data->size()));
      }
      c++;
    }

    if (!bw.byte_aligned()) {
      return td::Status::Error("Not byte aligned");
    }

    return std::string(buffer, bw.get_byte_ptr());
  } catch (vm::VmError& err) {
    return td::Status::Error(PSLICE() << "Exception while parsing chunks data: " << err.get_msg());
  }
}

td::Result<std::string> ton_http::utils::parse_content_data(td::Ref<vm::CellSlice> cs) {
  switch (tokens::gen::t_ContentData.check_tag(*cs)) {
    case tokens::gen::ContentData::snake: {
      tokens::gen::ContentData::Record_snake snake_record;
      if (!tlb::csr_unpack(cs, snake_record)) {
        return td::Status::Error("Failed to unpack snake token data");
      }
      return parse_snake_data(snake_record.data);
    }
    case tokens::gen::ContentData::chunks: {
      tokens::gen::ContentData::Record_chunks chunks_record;
      if (!tlb::csr_unpack(cs, chunks_record)) {
        return td::Status::Error("Failed to unpack chunks token data");
      }
      return parse_chunks_data(chunks_record.data);
    }
    default:
      return td::Status::Error("Unknown content data");
  }
}

td::Result<std::tuple<bool, std::map<std::string, std::string>>> ton_http::utils::parse_token_data(td::Ref<vm::Cell> cell) {
  static std::string attributes[] = {"uri", "name", "description", "image", "image_data", "symbol", "decimals", "amount_style", "render_type"};

  try {
    auto cs = vm::load_cell_slice_ref(cell);
    switch (tokens::gen::t_FullContent.check_tag(*cs)) {
      case tokens::gen::FullContent::offchain: {
        tokens::gen::FullContent::Record_offchain offchain_record;
        if (!tlb::csr_unpack(cs, offchain_record)) {
          return td::Status::Error("Failed to unpack offchain token data");
        }
        auto uri_r = parse_snake_data(offchain_record.uri);
        if (uri_r.is_error()) {
          return uri_r.move_as_error();
        }
        auto uri = uri_r.move_as_ok();
        if (!td::check_utf8(uri)) {
          return td::Status::Error("Invalid uri");
        }
        return std::make_tuple(false, std::map<std::string, std::string>{{"uri", std::move(uri)}});
      }
      case tokens::gen::FullContent::onchain: {
        tokens::gen::FullContent::Record_onchain onchain_record;
        if (!tlb::csr_unpack(cs, onchain_record)) {
          return td::Status::Error("Failed to unpack onchain token data");
        }

        vm::Dictionary dict(onchain_record.data, 256);
        std::map<std::string, std::string> res;

        for (auto attr : attributes) {
          auto value_cs = dict.lookup(td::sha256_bits256(attr));
          if (value_cs.not_null()) {
            // TEP-64 standard requires that all attributes are stored in a dictionary with a single reference as a value:
            //    onchain#00 data:(HashmapE 256 ^ContentData) = FullContent;
            // however, some contracts store attributes as a direct value:
            //    onchain#00 data:(HashmapE 256 ContentData) = FullContent;
            // so we need to handle both cases.
            if (value_cs->size() == 0 && value_cs->size_refs() == 1) {
              value_cs = vm::load_cell_slice_ref(value_cs->prefetch_ref());
            }

            auto attr_data_r = parse_content_data(value_cs);
            if (attr_data_r.is_error()) {
              LOG(ERROR) << "Failed to parse attribute " << attr << ": " << attr_data_r.move_as_error().message();
              continue;
            }
            auto attr_data = attr_data_r.move_as_ok();
            if (attr == "image_data") {
              res[attr] = td::base64_encode(attr_data);
            } else {
              if (!td::check_utf8(attr_data)) {
                LOG(ERROR) << "Invalid data (not utf8) in attribute " << attr;
                continue;
              }
              res[attr] = std::move(attr_data);
            }
          }
        }
        return std::make_tuple(true, res);
      }
      default:
        return td::Status::Error("Unknown token data type");
    }
  } catch (vm::VmError& err) {
    return td::Status::Error(PSLICE() << "Failed to parse token data: " << err.get_msg());
  }
}

