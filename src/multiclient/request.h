#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

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

struct RequestJson {
  RequestParameters parameters;
  std::string request;
};

}  // namespace multiclient
