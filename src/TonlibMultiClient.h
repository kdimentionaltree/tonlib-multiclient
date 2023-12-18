#pragma once
#include <map>

#include "td/actor/actor.h"
#include "common/delay.h"
#include "auto/tl/tonlib_api.hpp"
#include "tonlib/TonlibClientWrapper.h"


class TonlibMultiClient: public td::actor::Actor {
public:
    struct WorkerInfo {
        std::int32_t ls_index;
        td::actor::ActorOwn<tonlib::TonlibClientWrapper> client_;

        bool enabled = true;
        bool archival = false;
        std::int32_t last_mc_seqno = 0;
        std::int32_t count_requests = 0;
        bool is_waiting_for_update = false;
    };

    TonlibMultiClient(std::string global_config_str, std::string keystore_dir);

    void start_up() override;
    void alarm() override;
private:
    std::string global_config_str_;
    std::string keystore_dir_;
    std::int32_t consensus_block_seqno_;
    std::map<std::int32_t, WorkerInfo> workers_;
    
    void got_worker_update(std::int32_t ls_index, tonlib_api::object_ptr<tonlib_api::blocks_masterchainInfo> info);
    void got_worker_update_error(std::int32_t ls_index);
    void compute_consensus_block();
};
