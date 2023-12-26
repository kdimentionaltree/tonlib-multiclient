#include "MultiClient.h"
#include "TonlibMultiClient.h"

#include "tonlib/TonlibCallback.h"

#include "td/utils/logging.h"
#include "td/utils/check.h"
#include "td/actor/actor.h"


class MultiClient::Impl {
public:
    Impl() {
        LOG(INFO) << "MultiClient::Impl was constructed";
    }

    void send(MultiClient::Request request) {
        if (request.id == 0 || request.function == nullptr) {
            LOG(ERROR) << "Drop wrong request " << request.id;
            return;
        }
        LOG(INFO) << "Running some request with id " << request.id;
        return;
    }

    MultiClient::Response receive(double timeout) {
        // VLOG(tonlib_requests) << "Begin to wait for updates with timeout " << timeout;
        // auto is_locked = receive_lock_.exchange(true);
        // CHECK(!is_locked);
        // auto response = receive_unlocked(timeout);
        // is_locked = receive_lock_.exchange(false);
        // CHECK(is_locked);
        // VLOG(tonlib_requests) << "End to wait for updates, returning object " << response.id << ' '
        //                       << response.object.get();
        // return response;
        return MultiClient::Response();
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;
    ~Impl() {
        LOG(INFO) << "MultiClient::Impl was destroyed";
    }
private:
    td::actor::Scheduler scheduler_{{1}};
    td::thread scheduler_thread_;
};

// MultiClient
MultiClient::MultiClient() : impl_(std::make_unique<Impl>()) {}

void MultiClient::send(Request&& request) {
  impl_->send(std::move(request));
}

MultiClient::Response MultiClient::receive(double timeout) {
  return impl_->receive(timeout);
}

MultiClient::Response MultiClient::execute(Request&& request) {
  Response response;
  response.id = request.id;
  response.object = tonlib::TonlibClient::static_request(std::move(request.function));
  return response;
}

MultiClient::~MultiClient() = default;
MultiClient::MultiClient(MultiClient&& other) = default;
MultiClient& MultiClient::operator=(MultiClient&& other) = default;
