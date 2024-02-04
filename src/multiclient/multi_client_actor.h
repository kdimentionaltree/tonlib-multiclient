#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "client_wrapper.h"
#include "multiclient/promise.h"
#include "request.h"
#include "td/actor/ActorOwn.h"
#include "td/actor/common.h"
#include "td/utils/Time.h"

namespace multiclient {

struct MultiClientActorConfig {
  std::filesystem::path global_config_path;
  std::optional<std::filesystem::path> key_store_root;
  std::string blockchain_name = "mainnet";
  bool reset_key_store = false;

  size_t max_consecutive_alive_check_errors = 10;
};

class MultiClientActor : public td::actor::Actor {
public:
  explicit MultiClientActor(MultiClientActorConfig config) : config_(std::move(config)){};

  void start_up() final;
  void alarm() final;

  template <typename T>
  void send_request(Request<T> request, td::Promise<typename T::ReturnType> promise);

private:
  struct WorkerInfo {
    td::actor::ActorOwn<ClientWrapper> id;
    bool is_alive = false;
    bool is_archival = false;
    int32_t last_mc_seqno = -1;

    bool is_waiting_for_update = false;
    size_t check_retry_count = 0;
    std::optional<td::Timestamp> check_retry_after = std::nullopt;
  };

  template <typename T>
  void send_worker_request(size_t worker_index, T&& request, td::Promise<typename T::ReturnType> promise) {
    td::actor::send_closure(
        workers_[worker_index].id, &ClientWrapper::send_request<T>, std::move(request), std::move(promise)
    );
  }

  std::vector<size_t> select_workers(const RequestParameters& options) const;

  void check_alive();
  void on_alive_checked(size_t worker_index, std::optional<int32_t> last_mc_seqno);

  void check_archival();
  void on_archival_checked(size_t worker_index, bool is_archival);

  const MultiClientActorConfig config_;
  std::vector<WorkerInfo> workers_;
  td::Timestamp next_archival_check_ = td::Timestamp::now();
};

template <typename T>
void MultiClientActor::send_request(Request<T> request, td::Promise<typename T::ReturnType> promise) {
  auto worker_indices = select_workers(request.parameters);
  if (worker_indices.empty()) {
    promise.set_error(td::Status::Error("No workers available"));
    return;
  }

  auto multi_promise = PromiseSuccessAny<typename T::ReturnType>(std::move(promise));
  for (auto worker_index : worker_indices) {
    send_worker_request<T>(worker_index, request.request_creator(), multi_promise.get_promise());
  }
}

}  // namespace multiclient
