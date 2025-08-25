#pragma once
#include <auto/tl/tonlib_api.h>
#include <auto/tl/tonlib_api_json.h>
#include <string>
#include <userver/formats/json.hpp>
#include "td/utils/Status.h"
#include "vm/cells/Cell.h"
#include "vm/cells/CellSlice.h"
#include "td/utils/base64.h"
#include "td/utils/misc.h"

namespace ton_http::utils {
using namespace ton;

// common
template <typename IntType>
std::optional<IntType> stringToInt(const std::string& str) {
  static_assert(std::is_integral<IntType>::value, "Template parameter must be an integral type");
  try {
    if constexpr (std::is_same<IntType, int>::value) {
      return static_cast<IntType>(std::stoi(str));
    } else if constexpr (std::is_same<IntType, long>::value) {
      return static_cast<IntType>(std::stol(str));
    } else if constexpr (std::is_same<IntType, long long>::value) {
      return static_cast<IntType>(std::stoll(str));
    } else if constexpr (std::is_same<IntType, unsigned long>::value) {
      return static_cast<IntType>(std::stoul(str));
    } else if constexpr (std::is_same<IntType, unsigned long long>::value) {
      return static_cast<IntType>(std::stoull(str));
    } else if constexpr (std::is_same<IntType, unsigned int>::value) {
      return static_cast<IntType>(std::stoul(str));
    } else if constexpr (std::is_same<IntType, short>::value) {
      return static_cast<IntType>(std::stoi(str));
    } else if constexpr (std::is_same<IntType, unsigned short>::value) {
      return static_cast<IntType>(std::stoul(str));
    } else {
      throw std::invalid_argument("Unsupported integer type");
    }
  } catch (...) {
    return std::nullopt;
  }
}

inline std::optional<bool> stringToBool(std::string str) {
  if (str.empty()) {
    return std::nullopt;
  }
  std::ranges::transform(str, str.begin(), ::tolower);
  if (str == "y" || str == "yes" || str == "t" || str == "true" || str == "on" || str == "1") {
    return true;
  }
  if (str == "n" || str == "no" || str == "f" || str == "false" || str == "off" || str == "0") {
    return false;
  } return std::nullopt;
}

inline std::optional<std::string> stringToHash(const std::string& str) {
  if (str.empty()) {
    return str;
  }

  if (str.length() == 44) {
    if (auto res = td::base64_decode(str); res.is_ok()) {
      return res.move_as_ok();
    }
    if (auto res = td::base64url_decode(str); res.is_ok()) {
      return res.move_as_ok();
    }
  } else if (str.length() == 43) {
    if (auto res = td::base64url_decode(str); res.is_ok()) {
      return res.move_as_ok();
    }
  } else if (str.length() == 64) {
    if (auto res = td::hex_decode(str); res.is_ok()) {
      return res.move_as_ok();
    }
  }
  return std::nullopt;
}

// run get method tools
td::Result<userver::formats::json::Value> render_tvm_stack(const std::string& stack_str);
td::Result<userver::formats::json::Value> render_tvm_element(const std::string& element_type, const userver::formats::json::Value& element);

userver::formats::json::Value serialize_tvm_stack(std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>& tvm_stack);
userver::formats::json::Value serialize_tvm_entry(tonlib_api::tvm_stackEntryCell& entry);
userver::formats::json::Value serialize_tvm_entry(tonlib_api::tvm_stackEntrySlice& entry);

td::Result<std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>> parse_stack(const std::vector<std::string>& stack_vector);

userver::formats::json::Value serialize_cell(td::Ref<vm::Cell>& cell);

td::Result<std::string> address_from_cell(std::string data);

td::Result<std::string> address_from_tvm_stack_entry(tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>& entry);
td::Result<std::string> number_from_tvm_stack_entry(tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>& entry);

// tokens
td::Result<std::string> parse_snake_data(td::Ref<vm::CellSlice> data);
td::Result<std::string> parse_chunks_data(td::Ref<vm::CellSlice> data);
td::Result<std::string> parse_content_data(td::Ref<vm::CellSlice> cs);
td::Result<std::tuple<bool, std::map<std::string, std::string>>> parse_token_data(td::Ref<vm::Cell> cell);
}