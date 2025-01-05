
#include "multi_client.h"
#include "multi_client_actor.h"
#include "request.h"
#include "response_callback.h"
#include "td/actor/actor.h"
#include "td/actor/common.h"

namespace multiclient {

MultiClient::MultiClient(MultiClientConfig config, std::unique_ptr<ResponseCallback> callback) :
    config_(std::move(config)),
    scheduler_(
        std::make_shared<td::actor::Scheduler>(std::vector<td::actor::Scheduler::NodeInfo>{config.scheduler_threads})
    ) {
  scheduler_->run_in_context_external([this, cb = std::move(callback)]() mutable {
    client_ = td::actor::create_actor<MultiClientActor>(
        "multiclient",
        MultiClientActorConfig{
            .global_config_path = config_.global_config_path,
            .key_store_root = config_.key_store_root,
            .blockchain_name = config_.blockchain_name,
            .reset_key_store = config_.reset_key_store,
        },
        std::move(cb)
    );
  });
  scheduler_thread_ = std::thread([scheduler = scheduler_] { scheduler->run(); });
}

MultiClient::~MultiClient() {
  scheduler_->stop();
}

td::Result<std::string> MultiClient::send_request_json(RequestJson req) const {
  std::promise<td::Result<std::string>> request_promise;
  auto request_future = request_promise.get_future();

  auto promise = td::Promise<std::string>([p = std::move(request_promise)](auto result) mutable {
    p.set_value(std::move(result));
  });

  scheduler_->run_in_context_external([this, p = std::move(promise), req = std::move(req)]() mutable {
    td::actor::send_closure(client_.get(), &MultiClientActor::send_request_json, std::move(req), std::move(p));
  });

  return request_future.get();
}

void MultiClient::send_callback_request(RequestCallback req) const {
  scheduler_->run_in_context_external([this, req = std::move(req)]() mutable {
    td::actor::send_closure(client_.get(), &MultiClientActor::send_callback_request, std::move(req));
  });
}
td::Result<std::int32_t> MultiClient::get_consensus_block() const {
  std::promise<td::Result<std::int32_t>> request_promise;
  auto request_future = request_promise.get_future();

  auto promise = td::Promise<std::int32_t>([p = std::move(request_promise)](auto result) mutable {
    p.set_value(std::move(result));
  });

  scheduler_->run_in_context_external([this, p = std::move(promise)]() mutable {
    td::actor::send_closure(client_.get(), &MultiClientActor::get_consensus_block, std::move(p));
  });

  return request_future.get();
}

}  // namespace multiclient
