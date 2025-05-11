#include "tvm_utils.h"

#include "auto/tl/tonlib_api.hpp"
#include "auto/tl/tonlib_api_json.h"

#include "common/refint.h"
#include "overlay/overlay-broadcast.hpp"
#include "td/utils/JsonBuilder.h"
#include "td/utils/Status.h"
#include "td/utils/logging.h"
#include "td/utils/overloaded.h"
#include "vm/boc.h"
#include "vm/cells/CellSlice.h"


std::string to_hex_string_with_prefix(td::RefInt256& val) {
  auto res = val->to_hex_string();
  if (res[0] == '-') {
    return "-0x" + res.substr(1, res.size() - 1);
  }
  return "0x" + res;
}


td::Result<userver::formats::json::Value>  ton_http::tvm::render_tvm_stack(const std::string& stack_str) {
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


td::Result<userver::formats::json::Value> ton_http::tvm::render_tvm_element(
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


userver::formats::json::Value ton_http::tvm::serialize_tvm_entry(tonlib_api::tvm_stackEntryCell& entry) {
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


userver::formats::json::Value ton_http::tvm::serialize_tvm_entry(tonlib_api::tvm_stackEntrySlice& entry) {
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

userver::formats::json::Value ton_http::tvm::serialize_cell(td::Ref<vm::Cell>& cell) {
  using namespace userver::formats::json;
  ValueBuilder builder;

  auto cs = vm::load_cell_slice(cell);
  auto r_ls = cell->load_cell();
  if (r_ls.is_error()) {
    builder["error"] = r_ls.move_as_error().to_string();
    return builder.ExtractValue();
  }
  auto ls = r_ls.move_as_ok();;
  builder["data"]["b64"] = td::base64_encode(serialize_data_cell(ls.data_cell));
  builder["data"]["len"] = ls.data_cell->get_bits();
  builder["refs"] = FromString("[]");
  builder["special"] = ls.data_cell->is_special();

  while (cs.have_refs(1)) {
    cs.advance(1);
    auto loc = cs.fetch_ref();
    LOG(ERROR) << "Fetched ref!";
    builder["refs"].PushBack(serialize_cell(loc));
  }
  LOG(ERROR) << "OK!";

  return builder.ExtractValue();
}


userver::formats::json::Value ton_http::tvm::serialize_tvm_stack(
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
      },
      [&](tonlib_api::tvm_stackEntryList& val) {
        entry_builder.PushBack("list");
      },
      [&](tonlib_api::tvm_stackEntryUnsupported& val) {
        entry_builder.PushBack("unsupported");
        entry_builder.PushBack("");
      }
      ));
    builder.PushBack(entry_builder.ExtractValue());
  }
  return builder.ExtractValue();
}
