#include "TonlibMultiClient.h"
#include "td/utils/logging.h"
#include "auto/tl/ton_api_json.h"
#include "td/utils/port/path.h"
#include "td/utils/Status.h"
#include "td/utils/JsonBuilder.h"

using namespace ton::tonlib_api;

TonlibMultiClient::TonlibMultiClient(std::string global_config_str, std::string keystore_dir) : 
    global_config_str_(global_config_str), keystore_dir_(keystore_dir), consensus_block_seqno_(0) {
}

void TonlibMultiClient::start_up() {
    auto config = tonlib::Config::parse(global_config_str_).move_as_ok();
    int n_clients = config.lite_clients.size();
    LOG(INFO) << "Count of LiteServers in the config: " << n_clients;
    
    for(int ls_index = 0; ls_index < n_clients; ++ls_index) {
        td::StringBuilder keystore_dir_builder;
        keystore_dir_builder << keystore_dir_ << "/ls_" << ls_index;
        auto keystore_dir_loc = keystore_dir_builder.as_cslice().str();
        td::mkdir(keystore_dir_loc).ensure();

        // build config for single liteserver
        auto config_json = td::json_decode(global_config_str_).move_as_ok();
        auto liteservers = get_json_object_field(config_json.get_object(), "liteservers", td::JsonValue::Type::Array, false).move_as_ok();
        auto &ls_array = liteservers.get_array();
        
        std::string result_ls_array;
        {
            td::JsonBuilder builder;
            auto arr = builder.enter_array();
            arr << ls_array[ls_index];
            arr.leave();
            result_ls_array = builder.string_builder().as_cslice().str();
        }
        
        std::string result_config_str;
        {
            td::JsonBuilder builder;
            auto obj = builder.enter_object();
            obj("dht", get_json_object_field(config_json.get_object(), "dht", td::JsonValue::Type::Object).move_as_ok());
            obj("@type", get_json_object_field(config_json.get_object(), "@type", td::JsonValue::Type::String).move_as_ok());
            obj("validator", get_json_object_field(config_json.get_object(), "validator", td::JsonValue::Type::Object).move_as_ok());
            obj("liteservers", td::JsonRaw(result_ls_array));
            obj.leave();
            result_config_str = builder.string_builder().as_cslice().str();
        }

        auto options = tonlib_api::make_object<tonlib_api::options>(
            tonlib_api::make_object<tonlib_api::config>(result_config_str, "", false, false),
            tonlib_api::make_object<tonlib_api::keyStoreTypeDirectory>(keystore_dir_loc));
        
        td::StringBuilder name;
        name << "tonlib_" << ls_index;
        LOG(INFO) << "Name: " << name.as_cslice().str() << " Keystore: " << keystore_dir_loc;
        workers_[ls_index] = {ls_index,  td::actor::create_actor<tonlib::TonlibClientWrapper>(td::actor::ActorOptions().with_name(name.as_cslice().str()).with_poll(), std::move(options))};
    }
    alarm_timestamp() = td::Timestamp::in(1.0);
}

void TonlibMultiClient::alarm() {
    td::StringBuilder builder;
    builder << "Consesus: " << consensus_block_seqno_ << ". Workers: ";
    for(auto& [ls_index, worker_] : workers_) {
        if (worker_.enabled && !worker_.is_waiting_for_update) {
            builder << "1";

            worker_.is_waiting_for_update = true;
            auto getMasterchainInfo = tonlib_api::make_object<tonlib_api::blocks_getMasterchainInfo>();
            auto P = td::PromiseCreator::lambda(
                [SelfId = actor_id(this), ls_index_ = ls_index](td::Result<tonlib_api::object_ptr<tonlib_api::blocks_masterchainInfo>> R) mutable {
                    if(R.is_error()) {
                        LOG(DEBUG) << "Dead worker #" << ls_index_ << ": " << R.move_as_error();
                        td::actor::send_closure(SelfId, &TonlibMultiClient::got_worker_update_error, ls_index_);
                        return;
                    }
                    td::actor::send_closure(SelfId, &TonlibMultiClient::got_worker_update, ls_index_, R.move_as_ok());
                });
            td::actor::send_closure(worker_.client_, &tonlib::TonlibClientWrapper::send_request<tonlib_api::blocks_getMasterchainInfo>, std::move(getMasterchainInfo), std::move(P));
        } else {
            builder << "0";
            LOG(DEBUG) << "Worker #" << ls_index << " is dead or still waiting for update";
        }
    }
    ton::delay_action([SelfId = actor_id(this)]() {
        td::actor::send_closure(SelfId, &TonlibMultiClient::compute_consensus_block);
    }, td::Timestamp::in(1.0));
    LOG(INFO) << builder.as_cslice().str();
    alarm_timestamp() = td::Timestamp::in(1.0);
}

void TonlibMultiClient::got_worker_update(std::int32_t ls_index, tonlib_api::object_ptr<tonlib_api::blocks_masterchainInfo> info) {
    workers_[ls_index].is_waiting_for_update = false;
    workers_[ls_index].last_mc_seqno = info->last_->seqno_;
    workers_[ls_index].enabled = true;  // TODO: compute consensus block
    LOG(DEBUG) << "LS #" << ls_index << ") Last masterchain seqno: " << info->last_->seqno_;
}

void TonlibMultiClient::got_worker_update_error(std::int32_t ls_index) {
    workers_[ls_index].is_waiting_for_update = false;
    workers_[ls_index].enabled = false;
    LOG(WARNING) << "Dead worker #" << ls_index << " was disabled";
}

void TonlibMultiClient::compute_consensus_block() {
    // FIXME: implement consensus block
    std::int32_t new_consensus_block = 0;
    for(auto& [ls_index, worker_] : workers_) {
        if (worker_.last_mc_seqno > new_consensus_block) 
            new_consensus_block = worker_.last_mc_seqno;
    }
    consensus_block_seqno_ = new_consensus_block;
}
