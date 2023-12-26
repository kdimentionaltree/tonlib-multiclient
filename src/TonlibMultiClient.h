#pragma once
#include <map>
#include <vector>
#include <random>

#include "td/actor/actor.h"
#include "common/delay.h"
#include "auto/tl/tonlib_api.hpp"
#include "tonlib/TonlibClientWrapper.h"
#include "tonlib/TonlibCallback.h"


constexpr std::int32_t MAX_RETRY_COUNT = 2;

struct RequestOptions {
    enum Mode {
        Single = 0,
        Broadcast = 1,
        Multiple = 2
    };
    Mode mode = Mode::Single;
    std::int32_t ls_index = -1;
    std::int32_t num_clients = 1;
    std::int32_t archival = -1;
};
struct SingleResponse { 
    std::int32_t ls_index;
    tonlib_api::object_ptr<tonlib_api::Object> result;
    tonlib_api::object_ptr<tonlib_api::error> error;
};
struct Response {
    std::uint64_t id;
    std::vector<SingleResponse> results;
};

class TonlibMultiClient: public td::actor::Actor {
public:
    struct WorkerInfo {
        std::int32_t ls_index;
        td::actor::ActorOwn<tonlib::TonlibClientWrapper> client_;

        bool enabled = false;
        bool archival = false;
        std::int32_t last_mc_seqno = 0;
        std::int32_t count_requests = 0;
        
        bool is_waiting_for_update = false;
        std::int32_t retry_count = 0;
        td::Timestamp retry_after = td::Timestamp::at(0);
    };

    TonlibMultiClient(std::string global_config_str, std::string keystore_dir, td::unique_ptr<tonlib::TonlibCallback> callback);

    void start_up() override;
    void alarm() override;

    void request(std::uint64_t id, tonlib_api::object_ptr<tonlib_api::Function> object, RequestOptions options = RequestOptions());
private:
    std::string global_config_str_;
    std::string keystore_dir_;
    td::unique_ptr<tonlib::TonlibCallback> callback_;
    
    std::int32_t consensus_block_seqno_;
    td::Timestamp next_archival_check_;

    std::map<std::int32_t, WorkerInfo> workers_;
    std::vector<std::int32_t> active_workers_;
    std::vector<std::int32_t> active_nonarchival_workers_;
    std::vector<std::int32_t> active_archival_workers_;

    // stuff for random
    std::random_device random_device_;
    
    // methods
    td::actor::ActorOwn<tonlib::TonlibClientWrapper> create_tonlib_actor(std::int32_t ls_index);
    
    void test_method();

    void do_check_archival();
    void do_worker_update();
    void do_update_consensus_block();
    
    void got_archival_update(std::int32_t ls_index, bool archival);
    void got_worker_update(std::int32_t ls_index, tonlib_api::object_ptr<tonlib_api::blocks_masterchainInfo> info);
    void got_worker_update_error(std::int32_t ls_index);
    void got_request_response(std::uint64_t id, Response R);

    std::vector<std::int32_t> select_workers(RequestOptions options);
};
