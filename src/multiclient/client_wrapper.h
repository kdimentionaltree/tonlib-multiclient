#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include "td/actor/ActorOwn.h"
#include "td/actor/PromiseFuture.h"
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
  explicit ClientWrapper(ClientConfig config);

  void start_up() override;
  void alarm() override;

  template <typename T>
  void send_request(T&& req, td::Promise<typename T::ReturnType> promise);

private:
  void try_init();
  void on_inited();

  const ClientConfig config_;
  td::actor::ActorOwn<tonlib::TonlibClient> tonlib_client_;
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
