#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include "response_callback.h"
#include "td/actor/ActorOwn.h"
#include "td/actor/PromiseFuture.h"
#include "td/actor/actor.h"
#include "td/actor/common.h"
#include "tonlib/TonlibClient.h"

namespace multiclient {

struct ClientConfig {
  std::string global_config;
  std::optional<std::filesystem::path> key_store;
  std::string blockchain_name = "mainnet";
  bool use_callbacks_for_network = false;
  bool ignore_cache = false;
};

class ClientWrapper : public td::actor::Actor {
public:
  explicit ClientWrapper(ClientConfig config, std::shared_ptr<ResponseCallback> callback);
  explicit ClientWrapper(uint64_t client_id, ClientConfig config, std::shared_ptr<ResponseCallback> callback);

  void start_up() override;
  void alarm() override;

  template <typename T>
  void send_request(T&& req, td::Promise<typename T::ReturnType> promise);
  void send_callback_request(uint64_t request_id, ton::tonlib_api::object_ptr<ton::tonlib_api::Function>&& request);
  void send_request_json(uint64_t request_id, std::string req, td::Promise<std::string> promise);

private:
  void try_init();
  void on_inited();

  void on_cb_result(uint64_t id, tonlib_api::object_ptr<tonlib_api::Object> result);
  void on_cb_error(uint64_t id, tonlib_api::object_ptr<tonlib_api::error> error);

  const uint64_t client_id_;
  const ClientConfig config_;
  std::shared_ptr<ResponseCallback> callback_;
  td::actor::ActorOwn<tonlib::TonlibClient> tonlib_client_;

  std::unordered_map<uint64_t, td::Promise<std::string>> json_requests_;

  bool inited_ = false;
};

template <typename T>
void ClientWrapper::send_request(T&& req, td::Promise<typename T::ReturnType> promise) {
  td::actor::send_closure(
      tonlib_client_,
      &tonlib::TonlibClient::make_request<T, td::Promise<typename T::ReturnType>>,
      std::move(req),
      std::move(promise)
  );
}

}  // namespace multiclient
