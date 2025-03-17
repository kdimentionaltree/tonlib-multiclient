#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include "auto/tl/tonlib_api.h"
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
  std::string blockchain_name = "";
  bool use_callbacks_for_network = false;
  bool ignore_cache = false;
  bool sync_tonlib = true;
};

class ClientWrapper : public td::actor::Actor {
public:
  explicit ClientWrapper(ClientConfig config, std::shared_ptr<ResponseCallback> callback);
  explicit ClientWrapper(uint64_t client_id, ClientConfig config, std::shared_ptr<ResponseCallback> callback);

  void start_up() override;
  void alarm() override;

  template <typename T>
  void send_request(T&& req, td::Promise<typename T::ReturnType> promise);

  template <typename T>
  void send_request_function(ton::tonlib_api::object_ptr<T>&& req, td::Promise<typename T::ReturnType> promise);

  void send_callback_request(uint64_t request_id, ton::tonlib_api::object_ptr<ton::tonlib_api::Function>&& request);
  void send_request_json(std::string req, td::Promise<std::string> promise);

private:
  void try_init();
  void on_inited();
  void try_sync();
  void on_synced();

  void on_cb_result(uint64_t id, ton::tonlib_api::object_ptr<ton::tonlib_api::Object> result);
  void on_cb_error(uint64_t id, ton::tonlib_api::object_ptr<ton::tonlib_api::error> error);

  const uint64_t client_id_;
  const ClientConfig config_;
  std::shared_ptr<ResponseCallback> callback_;
  td::actor::ActorOwn<tonlib::TonlibClient> tonlib_client_;

  std::unordered_map<uint64_t, td::Promise<ton::tonlib_api::object_ptr<ton::tonlib_api::Object>>> tracking_requests_;

  bool inited_ = false;
  bool synced_ = false;
  size_t request_id_ = 100;
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

template <typename T>
void ClientWrapper::send_request_function(
    ton::tonlib_api::object_ptr<T>&& req, td::Promise<typename T::ReturnType> promise
) {
  auto request_id = request_id_++;
  tracking_requests_.emplace(request_id, [p = std::move(promise)](auto res) mutable {
    if (res.is_error()) {
      p.set_error(res.move_as_error());
    } else {
      p.set_value(ton::tonlib_api::move_object_as<typename T::ReturnType::element_type>(res.move_as_ok()));
    }
  });
  send_callback_request(request_id, std::move(req));
}

}  // namespace multiclient
