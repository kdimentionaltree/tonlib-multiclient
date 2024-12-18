#pragma once
#include "tonlib-multiclient/multi_client.h"
#include "tl/tl_json.h"
#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api_json.h"
#include "td/utils/JsonBuilder.h"
#include "tonlib-multiclient/request.h"

namespace ton_http::core {
struct TonlibWorkerResponse {
  bool is_ok{false};
  tonlib_api::object_ptr<tonlib_api::Object> result{nullptr};
  std::optional<td::Status> error{std::nullopt};
};

class TonlibWorker {
public:
  explicit TonlibWorker(const multiclient::MultiClientConfig& config) : tonlib_(config) {};
  ~TonlibWorker() = default;

  [[nodiscard]] TonlibWorkerResponse getMasterchainInfo() const;
private:
  multiclient::MultiClient tonlib_;
};
}
