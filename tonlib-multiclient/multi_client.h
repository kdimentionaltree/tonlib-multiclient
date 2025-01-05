#pragma once

#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <thread>
#include "auto/tl/tonlib_api.h"
#include "multi_client_actor.h"
#include "request.h"
#include "response_callback.h"
#include "td/actor/ActorId.h"
#include "td/actor/ActorOwn.h"
#include "td/actor/PromiseFuture.h"
#include "td/actor/actor.h"
#include "td/utils/Status.h"

namespace multiclient {

struct MultiClientConfig {
  std::filesystem::path global_config_path;
  std::optional<std::filesystem::path> key_store_root;
  std::string blockchain_name = "mainnet";
  bool reset_key_store = false;
  size_t scheduler_threads = 1;
};

class MultiClient {
public:
  explicit MultiClient(MultiClientConfig config, std::unique_ptr<ResponseCallback> callback = nullptr);
  ~MultiClient();

  template <typename T>
  td::Result<typename T::ReturnType> send_request(Request<T> req) const;

  template <typename T>
  td::Result<typename T::ReturnType> send_request_function(RequestFunction<T> req) const;

  td::Result<std::string> send_request_json(RequestJson req) const;
  void send_callback_request(RequestCallback req) const;

  td::Result<std::int32_t> get_consensus_block() const;
private:
  const MultiClientConfig config_;
  std::shared_ptr<td::actor::Scheduler> scheduler_;
  std::thread scheduler_thread_;
  td::actor::ActorOwn<MultiClientActor> client_;
};

using MultiClientPtr = std::unique_ptr<MultiClient>;

template <typename T>
td::Result<typename T::ReturnType> MultiClient::send_request(Request<T> req) const {
  using ReturnType = typename T::ReturnType;

  std::promise<td::Result<ReturnType>> request_promise;
  auto request_future = request_promise.get_future();

  auto promise = td::Promise<ReturnType>([p = std::move(request_promise)](auto result) mutable {
    p.set_value(std::move(result));
  });

  scheduler_->run_in_context_external([this, p = std::move(promise), req = std::move(req)]() mutable {
    td::actor::send_closure(client_.get(), &MultiClientActor::send_request<T>, std::move(req), std::move(p));
  });

  return request_future.get();
}

template <typename T>
td::Result<typename T::ReturnType> MultiClient::send_request_function(RequestFunction<T> req) const {
  using ReturnType = typename T::ReturnType;

  std::promise<td::Result<ReturnType>> request_promise;
  auto request_future = request_promise.get_future();

  auto promise = td::Promise<ReturnType>([p = std::move(request_promise)](auto result) mutable {
    p.set_value(std::move(result));
  });

  scheduler_->run_in_context_external([this, p = std::move(promise), req = std::move(req)]() mutable {
    td::actor::send_closure(client_.get(), &MultiClientActor::send_request_function<T>, std::move(req), std::move(p));
  });

  return request_future.get();
}


}  // namespace multiclient
