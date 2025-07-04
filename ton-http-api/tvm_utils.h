#pragma once
#include <auto/tl/tonlib_api.h>
#include <auto/tl/tonlib_api_json.h>
#include <string>
#include <userver/formats/json.hpp>
#include "td/utils/Status.h"
#include "vm/cells/Cell.h"

namespace ton_http::tvm {
using namespace ton;

td::Result<userver::formats::json::Value> render_tvm_stack(const std::string& stack_str);
td::Result<userver::formats::json::Value> render_tvm_element(const std::string& element_type, const userver::formats::json::Value& element);

userver::formats::json::Value serialize_tvm_stack(std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>& tvm_stack);
userver::formats::json::Value serialize_tvm_entry(tonlib_api::tvm_stackEntryCell& entry);
userver::formats::json::Value serialize_tvm_entry(tonlib_api::tvm_stackEntrySlice& entry);

td::Result<std::vector<tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>>> parse_stack(const std::string& stack_string);

userver::formats::json::Value serialize_cell(td::Ref<vm::Cell>& cell);

td::Result<std::string> address_from_cell(std::string data);

td::Result<std::string> address_from_tvm_stack_entry(tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>& entry);
td::Result<std::string> number_from_tvm_stack_entry(tonlib_api::object_ptr<tonlib_api::tvm_StackEntry>& entry);
}