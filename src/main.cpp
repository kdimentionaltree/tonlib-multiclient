#include "td/utils/port/signals.h"
#include "td/utils/port/path.h"
#include "td/utils/OptionParser.h"
#include "td/utils/logging.h"
#include "td/utils/check.h"
#include "td/utils/filesystem.h"

#include "td/actor/actor.h"

#include "tonlib/TonlibClient.h"
#include "TonlibMultiClient.h"


int main(int argc, char* argv[]) {
    SET_VERBOSITY_LEVEL(verbosity_INFO);
    td::set_default_failure_signal_handler().ensure();

    td::OptionParser p;
    std::string global_config_str;
    std::string keystore_dir = "private/test-keystore";
    bool reset_keystore_dir = false;
    
    p.add_checked_option('C', "global-config", "file to read global config", [&](td::Slice fname) {
        TRY_RESULT(str, td::read_file_str(fname.str()));
        global_config_str = std::move(str);
        return td::Status::OK();
    });
    p.add_option('f', "force", "reset keystore dir", [&]() { reset_keystore_dir = true; });
    p.run(argc, argv).ensure();

    if (reset_keystore_dir) {
        td::rmrf(keystore_dir).ignore();
    }
    td::mkdir(keystore_dir).ensure();
    LOG(INFO) << "Arguments were parsed";

    // options
    td::actor::ActorOwn<TonlibMultiClient> my_actor_;
    td::actor::Scheduler scheduler_({1});

    class Cb : public TonlibMultiClientCallback {
    public:
        explicit Cb() {}
        void on_result(std::uint64_t id, Response response) override {
            LOG(INFO) << "Callback for request #" << response.id << " with " << response.results.size() << " responses";
        }
        void on_error(std::uint64_t id, std::string error) override {
            LOG(ERROR) << "Callback for request #" << id << " with error: " << error;
        }
        ~Cb() override {
            LOG(INFO) << "Callback destructor";
        }
    };
    scheduler_.run_in_context([&] {
        my_actor_ = td::actor::create_actor<TonlibMultiClient>("TonlibMultiClient", global_config_str, keystore_dir, td::make_unique<Cb>());
    });
    scheduler_.run();
    return 0;
}
