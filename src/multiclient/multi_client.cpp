
#include "multi_client.h"
#include "multiclient/multi_client_actor.h"
#include "td/actor/actor.h"
#include "td/actor/common.h"

namespace multiclient {

MultiClient::MultiClient(MultiClientConfig config) :
    config_(std::move(config)),
    scheduler_(
        std::make_shared<td::actor::Scheduler>(std::vector<td::actor::Scheduler::NodeInfo>{config.scheduler_threads})
    ),
    scheduler_thread_([scheduler = scheduler_] { scheduler->run(); }) {
  scheduler_->run_in_context_external([this]() {
    client_ = td::actor::create_actor<MultiClientActor>(
        "multiclient",
        MultiClientActorConfig{
            .global_config_path = config_.global_config_path,
            .key_store_root = config_.key_store_root,
            .blockchain_name = config_.blockchain_name,
            .reset_key_store = config_.reset_key_store,
        }
    );
  });
}

MultiClient::~MultiClient() {
  scheduler_->stop();
}

}  // namespace multiclient
