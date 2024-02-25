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

// The reason for this struct is that `TonlibClient::make_request` cannot process several specific requests like
// `ton::tonlib_api::raw_getAccountState`.
// The main difference of `Request` that this struct uses `ton::tonlib_api::object_ptr<T>` instead of `T` for
// `request_creator`. This kind of requests is processed by the `TonlibClient::request` (`Request` —
// `TonlibClient::make_request`) function with callback handling under the hood of `ClientWrapper`.
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
