#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include "auto/tl/tonlib_api.h"

namespace multiclient {

enum class RequestMode {
  Single,
  Broadcast,
  Multiple,
};

struct RequestParameters {
  RequestMode mode = RequestMode::Single;
  std::optional<std::vector<size_t>> lite_server_indexes = std::nullopt;
  std::optional<size_t> clients_number = std::nullopt;
  bool archival = false;
};

template <typename T>
struct Request {
  using CreateTonlibRequestFunc = std::function<T()>;

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
