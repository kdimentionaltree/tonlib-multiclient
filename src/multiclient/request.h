#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include "auto/tl/tonlib_api.h"

namespace multiclient {

enum class RequestMode : uint8_t {
  Single,
  Broadcast,
  Multiple,
};

struct RequestParameters {
  RequestMode mode = RequestMode::Single;
  std::optional<std::vector<size_t>> lite_server_indexes = std::nullopt;
  std::optional<size_t> clients_number = std::nullopt;
  bool archival = false;

  bool are_valid() const {
    if (mode == RequestMode::Single) {
      return !lite_server_indexes->empty() && lite_server_indexes->size() == 1;
    }

    if (mode == RequestMode::Multiple) {
      return !(clients_number.has_value() && lite_server_indexes.has_value()) &&
          (clients_number.has_value() || lite_server_indexes.has_value());
    }

    return true;
  }
};

template <typename T>
struct Request {
  using CreateTonlibRequestFunc = std::function<T()>;

  RequestParameters parameters;
  CreateTonlibRequestFunc request_creator;
};

// Prefer using `Request` over `RequestFunction` whenever possible.
// This struct was introduced because `TonlibClient::make_request` is unable to process several specific requests, such
// as `ton::tonlib_api::raw_getAccountState`. A notable difference of `Request` is its utilization of
// `ton::tonlib_api::object_ptr<T>` rather than using `T` for the `request_creator`. This approach allows
// requests of this specific nature to be processed by the `TonlibClient::request` function, which employs callback
// handling within the `ClientWrapper`. This is in contrast to the `TonlibClient::make_request` function, which uses
// native promise handling.
template <typename T>
struct RequestFunction {
  using CreateTonlibRequestFunc = std::function<ton::tonlib_api::object_ptr<T>()>;

  RequestParameters parameters;
  CreateTonlibRequestFunc request_creator;
};

struct RequestCallback {
  using CreateTonlibCallbackRequestFunc = std::function<ton::tonlib_api::object_ptr<ton::tonlib_api::Function>()>;

  RequestParameters parameters;
  CreateTonlibCallbackRequestFunc request_creator;
  size_t request_id = 999;
};

struct RequestJson {
  RequestParameters parameters;
  std::string request;
};

}  // namespace multiclient
