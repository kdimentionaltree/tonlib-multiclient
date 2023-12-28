#include <algorithm>

#include "TonlibMultiClient.h"

#include "auto/tl/ton_api_json.h"
#include "auto/tl/tonlib_api_json.h"
#include "tl/tl_json.h"

#include "td/actor/MultiPromise.h"
#include "td/utils/common.h"
#include "td/utils/format.h"
#include "td/utils/logging.h"
#include "td/utils/port/path.h"
#include "td/utils/Status.h"
#include "td/utils/JsonBuilder.h"


using namespace ton::tonlib_api;

TonlibMultiClient::TonlibMultiClient(std::string global_config_str, std::string keystore_dir, td::unique_ptr<TonlibMultiClientCallback> callback) : 
    global_config_str_(global_config_str), 
    keystore_dir_(keystore_dir), 
    callback_(std::move(callback)),
    consensus_block_seqno_(0),
    next_archival_check_(td::Timestamp::now()) {
}

td::actor::ActorOwn<tonlib::TonlibClientWrapper> TonlibMultiClient::create_tonlib_actor(std::int32_t ls_index) {
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
    return td::actor::create_actor<tonlib::TonlibClientWrapper>(td::actor::ActorOptions().with_name(name.as_cslice().str()).with_poll(), std::move(options));
}

void TonlibMultiClient::test_method() {
    auto request = tonlib_api::make_object<tonlib_api::blocks_getMasterchainInfo>();
    RequestOptions options;
    options.mode = RequestOptions::Mode::Broadcast;
    // options.mode = RequestOptions::Mode::Single;
    options.ls_index = -1;
    options.archival = -1;
    std::uint64_t id = td::Timestamp::now().at_unix();
    td::actor::send_closure(actor_id(this), &TonlibMultiClient::request, id, std::move(request), options);
}

void TonlibMultiClient::start_up() {
    auto config = tonlib::Config::parse(global_config_str_).move_as_ok();
    int n_clients = config.lite_clients.size();
    LOG(INFO) << "Count of LiteServers in the config: " << n_clients;
    
    for(int ls_index = 0; ls_index < n_clients; ++ls_index) {
        workers_[ls_index] = {ls_index, create_tonlib_actor(ls_index)};
    }
    alarm_timestamp() = td::Timestamp::in(1.0);
    next_archival_check_ = td::Timestamp::in(2.0);
}

void TonlibMultiClient::do_worker_update() {
    // check workers alive
    for(auto& [ls_index, worker_] : workers_) {
        if ((worker_.enabled || worker_.retry_after.is_in_past()) && !worker_.is_waiting_for_update) {
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
            td::actor::send_closure(worker_.client_, 
                                    &tonlib::TonlibClientWrapper::send_request<tonlib_api::blocks_getMasterchainInfo>, 
                                    std::move(getMasterchainInfo), std::move(P));
        }
    }
    // update consensus block
    ton::delay_action([SelfId = actor_id(this)]() {
        td::actor::send_closure(SelfId, &TonlibMultiClient::do_update_consensus_block);
    }, td::Timestamp::in(1.0));
}

void TonlibMultiClient::alarm() {
    // do update worker inplace
    do_worker_update();

    // check if need archival update
    if (next_archival_check_.is_in_past()) {
        td::actor::send_closure(actor_id(this), &TonlibMultiClient::do_check_archival);
        next_archival_check_ = td::Timestamp::in(10.0);
        LOG(INFO) << "Checking archival workers";
    }
    
    // FIXME: debug method
    td::actor::send_closure(actor_id(this), &TonlibMultiClient::test_method);

    // print info 
    td::StringBuilder builder;
    builder << "Consesus: " << consensus_block_seqno_ << ". Workers: " ;
    for(auto& [ls_index, worker_] : workers_) {
        if (worker_.enabled) {
            if (worker_.archival) {
                builder << "2";
            } else {
                builder << "1";
            }
        } else {
            builder << "0";
        }
    }
    builder << ". Timestamp: " << td::Timestamp::now().at_unix();
    builder << ". LS count: " << active_workers_.size() << " = " 
            << active_nonarchival_workers_.size() << " + " 
            << active_archival_workers_.size();
    LOG(INFO) << builder.as_cslice().str();

    // next alarm
    alarm_timestamp() = td::Timestamp::in(1.0);
}

void TonlibMultiClient::do_check_archival() {
    for(auto& [ls_index, worker_] : workers_) {
        if (worker_.enabled) {
            int seqno = 3;
            auto P = td::PromiseCreator::lambda([SelfId = actor_id(this), ls_index_ = ls_index](td::Result<tonlib_api::object_ptr<tonlib_api::ton_blockIdExt>> R) {
                bool archival = (R.is_error() ? false : true);
                td::actor::send_closure(SelfId, &TonlibMultiClient::got_archival_update, ls_index_, archival);
            });
            auto lookupBlock = tonlib_api::make_object<tonlib_api::blocks_lookupBlock>(
                1, tonlib_api::make_object<tonlib_api::ton_blockId>(ton::masterchainId, ton::shardIdAll, seqno), 0, 0);
            td::actor::send_closure(worker_.client_, 
                &tonlib::TonlibClientWrapper::send_request<tonlib_api::blocks_lookupBlock>,
                std::move(lookupBlock), std::move(P));
        }
    }
}

void TonlibMultiClient::got_archival_update(std::int32_t ls_index, bool archival) {
    workers_[ls_index].archival = archival;
    LOG(DEBUG) << "LS #" << ls_index << " archival: " << archival;
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
    LOG(WARNING) << "Dead worker #" << ls_index << " was disabled for 10 seconds";
    workers_[ls_index].retry_after = td::Timestamp::in(10);
}

void TonlibMultiClient::do_update_consensus_block() {
    // FIXME: implement consensus block
    std::int32_t new_consensus_block = 0;
    for(auto& [ls_index, worker_] : workers_) {
        if (worker_.last_mc_seqno > new_consensus_block) 
            new_consensus_block = worker_.last_mc_seqno;
    }
    consensus_block_seqno_ = new_consensus_block;

    // update active_workers and active_archival_workers
    active_workers_.clear();
    active_nonarchival_workers_.clear();
    active_archival_workers_.clear();
    
    for(auto &[ls_index, worker_] : workers_) {
        if(worker_.enabled) {
            active_workers_.push_back(ls_index);
            if (worker_.archival) {
                active_archival_workers_.push_back(ls_index);
            } else {
                active_nonarchival_workers_.push_back(ls_index);
            }
        }
    }
}

void TonlibMultiClient::request(std::uint64_t id, tonlib_api::object_ptr<tonlib_api::Function> object, RequestOptions options) {
    auto object_json_str = td::json_encode<std::string>(td::ToJson(object), false);
    request_json(id, object_json_str, options);
}

void TonlibMultiClient::request_json_single(std::uint64_t id, std::int32_t ls_index, std::string object_json_str) {
    LOG(INFO) << "Request #" << id << " is single request";
    auto promise = td::PromiseCreator::lambda([id, ls_index, SelfId = actor_id(this)] (td::Result<tonlib_api::object_ptr<tonlib_api::Object>> R) mutable {
        Response response = {id};
        if (R.is_error()) {
            auto error = R.move_as_error();
            LOG(WARNING) << "LS #" << ls_index << " request #" << id << " failed:" << error;
            response.results.push_back({ls_index, nullptr, nullptr});
        } else {
            auto result = R.move_as_ok();
            LOG(DEBUG) << "LS #" << ls_index << " request #" << id << " ok:" << td::json_encode<std::string>(td::ToJson(*result.get()));
            if (result->get_id() == tonlib_api::error::ID) {
                response.results.push_back({ls_index, nullptr, tonlib_api::move_object_as<tonlib_api::error>(result)});
            } else {
                response.results.push_back({ls_index, std::move(result), nullptr});
            }
        }
        td::actor::send_closure(SelfId, &TonlibMultiClient::got_request_response, std::move(response));
    });
    tonlib_api::object_ptr<tonlib_api::Function> func;
    auto object_json = td::json_decode(object_json_str).move_as_ok();
        auto status = from_json(func, std::move(object_json));
        if (status.is_error()) {
            LOG(ERROR) << "Something went wrong in decoding of the encoded object";
            return;
        }
    td::actor::send_closure(workers_[ls_index].client_, &tonlib::TonlibClientWrapper::send_any_request, std::move(func), std::move(promise));
}

void TonlibMultiClient::request_json_multiple(std::uint64_t id, std::vector<std::int32_t> ls_index_list, std::string object_json_str) {
    // ugly copying the request
    auto response_ptr = std::make_shared<Response>();
    response_ptr->id = id;

    td::MultiPromise mp;
    auto ig = mp.init_guard();
    auto final_promise = td::PromiseCreator::lambda([response_ptr, SelfId = actor_id(this)](td::Result<td::Unit> R) {
        if (R.is_error()) {
            LOG(WARNING) << "Request #" << response_ptr->id << ". Final promise error: " << R.move_as_error();
        } else {
            LOG(DEBUG) << "Request #" << response_ptr->id << ". Final promise OK";
        }
        td::actor::send_closure(SelfId, &TonlibMultiClient::got_request_response, std::move(*response_ptr));
    });
    ig.add_promise(std::move(final_promise));

    for(auto ls_index : ls_index_list) {
        auto promise = td::PromiseCreator::lambda([ls_index, response_ptr, promise_ = ig.get_promise()] (td::Result<tonlib_api::object_ptr<tonlib_api::Object>> R) mutable {
            if (R.is_error()) {
                auto error = R.move_as_error();
                LOG(WARNING) << "LS #" << ls_index << " request #" << response_ptr->id << " failed:" << error;
                response_ptr->results.push_back({ls_index, nullptr, nullptr});
            } else {
                auto result = R.move_as_ok();
                LOG(DEBUG) << "LS #" << ls_index << " request #" << response_ptr->id << " ok:" << td::json_encode<std::string>(td::ToJson(*result.get()));
                if (result->get_id() == tonlib_api::error::ID) {
                    response_ptr->results.push_back({ls_index, nullptr, tonlib_api::move_object_as<tonlib_api::error>(result)});
                } else {
                    response_ptr->results.push_back({ls_index, std::move(result), nullptr});
                }
            }
            promise_.set_result(td::Unit());
        });
        tonlib_api::object_ptr<tonlib_api::Function> func;
        auto object_json = td::json_decode(object_json_str).move_as_ok();
        auto status = from_json(func, std::move(object_json));
        if (status.is_error()) {
            LOG(ERROR) << "Something went wrong in decoding of the encoded object";
            return;
        }
        td::actor::send_closure(workers_[ls_index].client_, &tonlib::TonlibClientWrapper::send_any_request, std::move(func), std::move(promise));
    }
}

void TonlibMultiClient::request_json(std::uint64_t id, std::string object_json_str, RequestOptions options) {
    auto ls_index_list = select_workers(options);
    LOG(DEBUG) << "Request " << id << " will be sent to " << ls_index_list.size() << " workers";

    if (ls_index_list.size() > 1) {
        request_json_multiple(id, ls_index_list, object_json_str);
        return;
    } else if (ls_index_list.size() == 1) {
        request_json_single(id, ls_index_list[0], object_json_str);
        return;
    } else {
        // TODO: handle error
        return;
    }
}

void TonlibMultiClient::got_request_response(Response R) {
    LOG(INFO) << "Got request #" << R.id << " response with " << R.results.size() << " results.";
    callback_->on_result(R.id, std::move(R));
}

std::vector<std::int32_t> TonlibMultiClient::select_workers(RequestOptions options) {
    std::vector<std::int32_t> result;
    switch (options.archival) {
            case 1:
                result = active_archival_workers_;
                break;
            case 0:
                result = active_nonarchival_workers_;
                break;
            default:
                result = active_workers_;
                break;
    }
    if (result.empty()) {
        return result;
    }
    if (options.mode == RequestOptions::Mode::Broadcast) {
        return result;
    } else if (options.mode == RequestOptions::Mode::Single) {
        if (options.ls_index >= 0) {
            if (std::find(result.begin(), result.end(), options.ls_index) != result.end())
                return std::vector<std::int32_t>{ options.ls_index };
        }
        auto rng = std::default_random_engine { random_device_() };
        std::uniform_int_distribution<> d(0, result.size());
        return std::vector<std::int32_t>{ result[d(rng)] };
    } else if (options.mode == RequestOptions::Mode::Multiple) {
        std::int32_t num_clients = options.num_clients;
        auto rng = std::default_random_engine { random_device_() };
        std::shuffle(std::begin(result), std::end(result), rng);

        if (num_clients < result.size())
            result.resize(num_clients);
        return result;
    }
    return result;
}
