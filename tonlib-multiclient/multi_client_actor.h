#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "auto/tl/tonlib_api.h"
#include "client_wrapper.h"
#include "promise.h"
#include "request.h"
#include "response_callback.h"
#include "td/actor/ActorOwn.h"
#include "td/actor/PromiseFuture.h"
#include "td/actor/common.h"
#include "td/utils/Time.h"

namespace multiclient {

struct MultiClientActorConfig {
  std::filesystem::path global_config_path;
  std::optional<std::filesystem::path> key_store_root;
  std::string blockchain_name = "";
  bool reset_key_store = false;

  size_t max_consecutive_alive_check_errors = 10;
};

class MultiClientActor : public td::actor::Actor {
public:
  explicit MultiClientActor(MultiClientActorConfig config, std::unique_ptr<ResponseCallback> callback = nullptr) :
      config_(std::move(config)), callback_(callback.release()) {
  }

  void start_up() final;
  void alarm() final;

  template <typename T>
  void send_request(Request<T> request, td::Promise<typename T::ReturnType> promise);

  template <typename T>
  void send_request_function(RequestFunction<T> req, td::Promise<typename T::ReturnType>);

  void send_request_json(RequestJson request, td::Promise<std::string> promise);
  void send_callback_request(RequestCallback request);

  size_t worker_count() const {
    return workers_.size();
  }
  void get_consensus_block(td::Promise<std::int32_t>&& promise);

  void get_session(const RequestParameters& params, SessionPtr&& session, td::Promise<SessionPtr>&& promise) {
    auto r_session = get_session_impl(params, session);
    promise.set_result(std::move(r_session));
  }

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

  template <typename T>
  void send_worker_request_function(
      size_t worker_index, ton::tonlib_api::object_ptr<T>&& request, td::Promise<typename T::ReturnType> promise
  ) {
    td::actor::send_closure(
        workers_[worker_index].id, &ClientWrapper::send_request_function<T>, std::move(request), std::move(promise)
    );
  }

  void send_worker_request_json(size_t worker_index, std::string request, td::Promise<std::string> promise) {
    td::actor::send_closure(
        workers_[worker_index].id, &ClientWrapper::send_request_json, std::move(request), std::move(promise)
    );
  }

  void send_worker_callback_request(
      size_t worker_index, uint64_t request_id, tonlib_api::object_ptr<tonlib_api::Function> request
  ) {
    td::actor::send_closure(
        workers_[worker_index].id, &ClientWrapper::send_callback_request, request_id, std::move(request)
    );
  }

  td::Result<SessionPtr> get_session_impl(const RequestParameters& options, SessionPtr session) const;
  std::vector<size_t> select_workers(const RequestParameters& options) const;

  void check_alive();
  void on_alive_checked(
      size_t worker_index, std::optional<int32_t> last_mc_seqno, bool first_archival_check_done = false
  );

  void check_archival(std::optional<size_t> check_worker_index = std::nullopt);
  void on_archival_checked(size_t worker_index, bool is_archival);

  const MultiClientActorConfig config_;
  std::shared_ptr<ResponseCallback> callback_;
  std::vector<WorkerInfo> workers_;
  bool first_archival_check_done_ = false;
  td::Timestamp next_archival_check_ = td::Timestamp::now();
  uint64_t json_request_id_ = 11;
};

template <typename T>
void MultiClientActor::send_request(Request<T> request, td::Promise<typename T::ReturnType> promise) {
  SessionPtr session;
  if (request.session) {
    session = request.session;
  } else {
    auto r_session = get_session_impl(request.parameters, nullptr);
    if (r_session.is_error()) {
      promise.set_error(r_session.move_as_error_prefix("failed to get session: "));
      return;
    }
    session = r_session.move_as_ok();
  }

  auto multi_promise = PromiseSuccessAny<typename T::ReturnType>(std::move(promise));
  for (auto worker_index : session->active_workers()) {
    send_worker_request<T>(worker_index, request.request_creator(), multi_promise.get_promise());
  }
}

template <typename T>
void MultiClientActor::send_request_function(RequestFunction<T> request, td::Promise<typename T::ReturnType> promise) {
  SessionPtr session;
  if (request.session) {
    session = request.session;
  } else {
    auto r_session = get_session_impl(request.parameters, nullptr);
    if (r_session.is_error()) {
      promise.set_error(r_session.move_as_error_prefix("failed to get session: "));
      return;
    }
    session = r_session.move_as_ok();
  }

  auto multi_promise = PromiseSuccessAny<typename T::ReturnType>(std::move(promise));
  for (auto worker_index : session->active_workers()) {
    send_worker_request_function<T>(worker_index, request.request_creator(), multi_promise.get_promise());
  }
}

}  // namespace multiclient
