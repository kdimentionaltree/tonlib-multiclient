#include "client_wrapper.h"
#include <cstdint>
#include <filesystem>
#include "auto/tl/tonlib_api.h"
#include "td/actor/ActorId.h"
#include "td/actor/PromiseFuture.h"
#include "td/actor/actor.h"
#include "td/utils/unique_ptr.h"
#include "tonlib/TonlibCallback.h"
#include "tonlib/TonlibClient.h"

namespace multiclient {

namespace {

class ClientWrapperEmptyCallback : public tonlib::TonlibCallback {
public:
  void on_result(std::uint64_t id, tonlib_api::object_ptr<tonlib_api::Object> result) final {
  }

  void on_error(std::uint64_t id, tonlib_api::object_ptr<tonlib_api::error> error) final {
  }

  ~ClientWrapperEmptyCallback() = default;
};

}  // namespace

ClientWrapper::ClientWrapper(ClientConfig config) : config_(std::move(config)) {
}

void ClientWrapper::start_up() {
  tonlib_client_ =
      td::actor::create_actor<tonlib::TonlibClient>("ClientWrapper", td::make_unique<ClientWrapperEmptyCallback>());
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
        }
      }
  );
}

void ClientWrapper::on_inited() {
  inited_ = true;
}


}  // namespace multiclient
