#include "client_wrapper.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api.hpp"
#include "auto/tl/tonlib_api_json.h"
#include "td/actor/ActorId.h"
#include "td/actor/PromiseFuture.h"
#include "td/actor/actor.h"
#include "td/utils/JsonBuilder.h"
#include "td/utils/unique_ptr.h"
#include "tl/tl_json.h"
#include "tonlib/TonlibCallback.h"
#include "tonlib/TonlibClient.h"

namespace multiclient {

ClientWrapper::ClientWrapper(uint64_t client_id, ClientConfig config, std::shared_ptr<ResponseCallback> callback) :
    td::actor::Actor(), client_id_(client_id), config_(std::move(config)), callback_(std::move(callback)) {
}

ClientWrapper::ClientWrapper(ClientConfig config, std::shared_ptr<ResponseCallback> callback) :
    ClientWrapper(0, std::move(config), std::move(callback)) {
}

void ClientWrapper::start_up() {
  class ClientWrapperCallback : public tonlib::TonlibCallback {
  public:
    ClientWrapperCallback(td::actor::ActorId<ClientWrapper> client_id) : client_id_(client_id) {
    }

    ~ClientWrapperCallback() = default;

    void on_result(uint64_t id, tonlib_api::object_ptr<tonlib_api::Object> result) final {
      td::actor::send_closure(client_id_, &ClientWrapper::on_cb_result, id, std::move(result));
    }

    void on_error(uint64_t id, tonlib_api::object_ptr<tonlib_api::error> error) final {
      td::actor::send_closure(client_id_, &ClientWrapper::on_cb_error, id, std::move(error));
    }

  private:
    td::actor::ActorId<ClientWrapper> client_id_;
  };

  tonlib_client_ = td::actor::create_actor<tonlib::TonlibClient>(
      "TonlibClient", td::make_unique<ClientWrapperCallback>(actor_id(this))
  );
  alarm();
}

void ClientWrapper::alarm() {
  static constexpr double kCheckInitedTimeout = 5.0;

  if (!inited_) {
    try_init();
    alarm_timestamp() = td::Timestamp::in(kCheckInitedTimeout);
  }
}

void ClientWrapper::try_init() {
  LOG(INFO) << "try init client";

  ton::tonlib_api::object_ptr<ton::tonlib_api::KeyStoreType> key_store =
      ton::tonlib_api::make_object<ton::tonlib_api::keyStoreTypeInMemory>();

  if (config_.key_store.has_value()) {
    std::filesystem::create_directories(*config_.key_store);
    key_store = ton::tonlib_api::make_object<ton::tonlib_api::keyStoreTypeDirectory>(config_.key_store->string());
  }

  send_request<ton::tonlib_api::init>(
      ton::tonlib_api::init(ton::tonlib_api::make_object<ton::tonlib_api::options>(
          ton::tonlib_api::make_object<ton::tonlib_api::config>(
              config_.global_config, config_.blockchain_name, config_.use_callbacks_for_network, config_.ignore_cache
          ),
          std::move(key_store)
      )),
      [self_id = actor_id(this)](auto res) {
        if (res.is_ok()) {
          td::actor::send_closure(self_id, &ClientWrapper::on_inited);
        } else {
          LOG(ERROR) << res.move_as_error_prefix("failed to init client: ");
        }
      }
  );
}

void ClientWrapper::on_inited() {
  inited_ = true;
  if (config_.sync_tonlib) {
    td::actor::send_closure(actor_id(this), &ClientWrapper::try_sync);
  } else {
    synced_ = true;
  }
}
void ClientWrapper::try_sync() {
  LOG(INFO) << "try sync tonlib";
  send_request_function<ton::tonlib_api::sync>(
    ton::tonlib_api::make_object<tonlib_api::sync>(),
    [self_id = actor_id(this)](auto res) {
      if (res.is_ok()) {
        td::actor::send_closure(self_id, &ClientWrapper::on_synced);
      }
  });
}
void ClientWrapper::on_synced() {
  synced_ = true;
}

void ClientWrapper::on_cb_result(uint64_t id, tonlib_api::object_ptr<tonlib_api::Object> result) {
  LOG(DEBUG) << "on_cb_result id: " << id;

  if (auto it = tracking_requests_.find(id); it != tracking_requests_.end()) {
    auto promise = std::move(it->second);
    tracking_requests_.erase(it);
    promise.set_result(std::move(result));
    return;
  }

  if (callback_ != nullptr) {
    callback_->on_result(client_id_, id, std::move(result));
  }
}

void ClientWrapper::on_cb_error(uint64_t id, tonlib_api::object_ptr<tonlib_api::error> error) {
  if (auto it = tracking_requests_.find(id); it != tracking_requests_.end()) {
    auto promise = std::move(it->second);
    tracking_requests_.erase(it);
    promise.set_error(td::Status::Error(error->message_));
    return;
  }

  if (callback_ != nullptr) {
    callback_->on_error(client_id_, id, std::move(error));
  }
}

void ClientWrapper::send_request_json(std::string request, td::Promise<std::string> promise) {
  auto request_id = request_id_++;
  auto object_json_res = td::json_decode(request);
  if (object_json_res.is_error()) {
    promise.set_error(td::Status::Error("Failed to decode json request from string"));
    return;
  }

  ton::tonlib_api::object_ptr<ton::tonlib_api::Function> func;
  auto status = td::from_json(func, object_json_res.move_as_ok());
  if (status.is_error()) {
    promise.set_error(td::Status::Error("Failed to parse request"));
    return;
  }

  tracking_requests_.emplace(request_id, promise.wrap([](auto result) {
    return td::json_encode<td::string>(td::ToJson(result));
  }));

  send_callback_request(request_id, std::move(func));
}

void ClientWrapper::send_callback_request(
    uint64_t request_id, ton::tonlib_api::object_ptr<ton::tonlib_api::Function>&& request
) {
  td::actor::send_closure(tonlib_client_, &tonlib::TonlibClient::request, request_id, std::move(request));
}

}  // namespace multiclient
