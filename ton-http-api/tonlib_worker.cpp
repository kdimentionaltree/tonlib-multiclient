#include "tonlib_worker.h"

namespace ton_http::core {
TonlibWorkerResponse TonlibWorker::getMasterchainInfo() const {
  auto request = multiclient::Request<ton::tonlib_api::blocks_getMasterchainInfo>{
    .parameters = {.mode = multiclient::RequestMode::Multiple, .clients_number = 1},
    .request_creator = [] {
      return ton::tonlib_api::blocks_getMasterchainInfo();
    },
  };
  auto result = tonlib_.send_request(std::move(request));
  TonlibWorkerResponse response;
  if (result.is_error()) {
    return TonlibWorkerResponse{false, nullptr, result.move_as_error()};
  }
  return TonlibWorkerResponse{true, result.move_as_ok(), std::nullopt};
}
}
