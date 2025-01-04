#include "multi_client_actor.h"
#include <algorithm>
#include <cstdint>
#include <random>
#include <ranges>
#include <string>
#include "auto/tl/tonlib_api.h"
#include "request.h"
#include "td/actor/PromiseFuture.h"
#include "td/actor/actor.h"
#include "td/utils/JsonBuilder.h"
#include "td/utils/check.h"
#include "td/utils/filesystem.h"

namespace multiclient {

namespace {

std::vector<std::string> split_global_config_by_liteservers(std::string global_config) {
  auto config_json = td::json_decode(global_config).move_as_ok();
  auto liteservers =
      get_json_object_field(config_json.get_object(), "liteservers", td::JsonValue::Type::Array, false).move_as_ok();
  const auto& ls_array = liteservers.get_array();

  std::vector<std::string> result;
  result.reserve(ls_array.size());

  for (const auto& ls_json : ls_array) {
    std::string result_ls_array;
    {
      td::JsonBuilder builder;
      auto arr = builder.enter_array();
      arr << ls_json;
      arr.leave();
      result_ls_array = builder.string_builder().as_cslice().str();
    }

    auto conf = td::json_decode(global_config).move_as_ok();

    std::string result_config_str;
    {
      td::JsonBuilder builder;
      auto obj = builder.enter_object();
      obj("dht", get_json_object_field(conf.get_object(), "dht", td::JsonValue::Type::Object).move_as_ok());
      obj("@type", get_json_object_field(conf.get_object(), "@type", td::JsonValue::Type::String).move_as_ok());
      obj("validator", get_json_object_field(conf.get_object(), "validator", td::JsonValue::Type::Object).move_as_ok());
      obj("liteservers", td::JsonRaw(result_ls_array));
      obj.leave();
      result_config_str = builder.string_builder().as_cslice().str();
    }

    result.push_back(std::move(result_config_str));
  }

  return result;
}

static auto kRandomDevice = std::random_device();
static auto kRandomEngine = std::default_random_engine(kRandomDevice());

template <typename T>
T get_random_index(T from, T to) {
  std::uniform_int_distribution<T> distribution(from, to);
  return distribution(kRandomEngine);
}

}  // namespace

void MultiClientActor::send_request_json(RequestJson request, td::Promise<std::string> promise) {
  auto worker_indices = select_workers(request.parameters);
  if (worker_indices.empty()) {
    promise.set_error(td::Status::Error("no workers available (" + request.parameters.to_string() + ")"));
    return;
  }
  auto multi_promise = PromiseSuccessAny<std::string>(std::move(promise));
  for (auto worker_index : worker_indices) {
    send_worker_request_json(worker_index, request.request, multi_promise.get_promise());
  }
}


void MultiClientActor::send_callback_request(RequestCallback request) {
  static constexpr size_t kUndefinedClientId = -1;

  CHECK(callback_ != nullptr);

  auto worker_indices = select_workers(request.parameters);
  if (worker_indices.empty()) {
    callback_->on_error(
        kUndefinedClientId, request.request_id,
        tonlib_api::make_object<tonlib_api::error>(400, "no workers available (" + request.parameters.to_string() + ")")
    );
    return;
  }

  for (auto worker_index : worker_indices) {
    send_worker_callback_request(worker_index, request.request_id, request.request_creator());
  }
}

void MultiClientActor::start_up() {
  static constexpr double kFirstAlarmAfter = 1.0;
  static constexpr double kCheckArchivalForFirstTimeAfter = 5.0;

  CHECK(std::filesystem::exists(config_.global_config_path));

  auto global_config = td::read_file_str(config_.global_config_path.string()).move_as_ok();
  auto config_splitted_by_liteservers = split_global_config_by_liteservers(std::move(global_config));

  CHECK(!config_splitted_by_liteservers.empty());

  if (config_.key_store_root.has_value()) {
    if (std::filesystem::exists(*config_.key_store_root)) {
      if (config_.reset_key_store) {
        std::filesystem::remove_all(*config_.key_store_root);
        std::filesystem::create_directories(*config_.key_store_root);
      }
    }
  }

  LOG(INFO) << "starting " << config_splitted_by_liteservers.size() << " client workers";

  for (size_t client_index = 0; client_index < config_splitted_by_liteservers.size(); client_index++) {
    workers_.push_back(WorkerInfo{
        .id = td::actor::create_actor<ClientWrapper>(
            td::actor::ActorOptions().with_name("multiclient_worker_" + std::to_string(client_index)).with_poll(),
            client_index,
            ClientConfig{
                .global_config = config_splitted_by_liteservers[client_index],
                .key_store = config_.key_store_root.has_value() ?
                    std::make_optional<std::filesystem::path>(
                        *config_.key_store_root / ("ls_" + std::to_string(client_index))
                    ) :
                    std::nullopt,
                .blockchain_name = config_.blockchain_name,
            },
            callback_
        ),
    });
  }

  alarm_timestamp() = td::Timestamp::in(kFirstAlarmAfter);
  next_archival_check_ = td::Timestamp::in(kCheckArchivalForFirstTimeAfter);
}

void MultiClientActor::alarm() {
  static constexpr double kDefaultAlarmInterval = 1.0;
  static constexpr double kCheckArchivalInterval = 2 * 60.0;

  LOG(DEBUG) << "Checking alive workers";
  check_alive();
  first_archival_check_done_ = true;

  if (next_archival_check_.is_in_past()) {
    check_archival();
    next_archival_check_ = td::Timestamp::in(kCheckArchivalInterval);
  }

  alarm_timestamp() = td::Timestamp::in(kDefaultAlarmInterval);
}

void MultiClientActor::check_alive() {
  for (size_t worker_index = 0; worker_index < workers_.size(); worker_index++) {
    auto& worker = workers_[worker_index];
    if (worker.is_waiting_for_update) {
      LOG(DEBUG) << "LS #" << worker_index << " is waiting for update";
      continue;
    }

    if (!worker.is_alive) {
      if (worker.check_retry_count > config_.max_consecutive_alive_check_errors) {
        LOG(DEBUG) << "LS #" << worker_index << " is dead, retry count exceeded";
        continue;
      }

      if (worker.check_retry_after.has_value() && worker.check_retry_after->is_in_past()) {
        LOG(DEBUG) << "LS #" << worker_index << " retrying check";
        worker.check_retry_count++;
        worker.check_retry_after = std::nullopt;
      } else if (worker.check_retry_count != 0) {
        LOG(DEBUG) << "LS #" << worker_index << " waiting for retry";
        continue;
      }
    }

    worker.is_waiting_for_update = true;
    send_worker_request<ton::tonlib_api::blocks_getMasterchainInfo>(
        worker_index,
        ton::tonlib_api::blocks_getMasterchainInfo(),
        [self_id = actor_id(this), worker_index, check_done = first_archival_check_done_](auto result) {
          td::actor::send_closure(
              self_id,
              &MultiClientActor::on_alive_checked,
              worker_index,
              result.is_ok() ? std::make_optional(result.ok()->last_->seqno_) : std::nullopt,
              check_done
          );
        }
    );
  }
}

void MultiClientActor::on_alive_checked(size_t worker_index, std::optional<int32_t> last_mc_seqno, bool first_archival_check_done) {
  static constexpr double kRetryInterval = 10.0;
  static constexpr int32_t kUndefinedLastMcSeqno = -1;

  bool is_alive = last_mc_seqno.has_value();
  int32_t last_mc_seqno_value = last_mc_seqno.value_or(kUndefinedLastMcSeqno);

  LOG(DEBUG) << "LS #" << worker_index << " is_alive: " << is_alive << " last_mc_seqno: " << last_mc_seqno_value;

  auto& worker = workers_[worker_index];
  worker.is_alive = is_alive;
  worker.is_waiting_for_update = false;

  if (is_alive) {
    worker.last_mc_seqno = last_mc_seqno_value;
    worker.check_retry_count = 0;
  } else {
    worker.check_retry_after = td::Timestamp::in(kRetryInterval);
  }

  if (is_alive && !first_archival_check_done) {
    LOG(INFO) << "First archival check for #" << worker_index << " liteserver";
    check_archival(worker_index);
  }
}

void MultiClientActor::check_archival(std::optional<size_t> check_worker_index) {
  static constexpr int32_t kBlockWorkchain = ton::masterchainId;
  static constexpr int64_t kBlockShard = ton::shardIdAll;
  static constexpr int32_t kBlockSeqno = 3;

  static constexpr int kLookupMode = 1;
  static constexpr int kLookupLt = 0;
  static constexpr int kLookupUtime = 0;

  for (size_t worker_index = 0; worker_index < workers_.size(); worker_index++) {
    if (!workers_[worker_index].is_alive) {
      continue;
    }
    if (check_worker_index.has_value() && check_worker_index.value() != worker_index) {
      continue;
    }

    send_worker_request<ton::tonlib_api::blocks_lookupBlock>(
        worker_index,
        ton::tonlib_api::blocks_lookupBlock(
            kLookupMode,
            ton::tonlib_api::make_object<ton::tonlib_api::ton_blockId>(kBlockWorkchain, kBlockShard, kBlockSeqno),
            kLookupLt,
            kLookupUtime
        ),
        [self_id = actor_id(this), worker_index](auto result) {
          td::actor::send_closure(self_id, &MultiClientActor::on_archival_checked, worker_index, result.is_ok());
        }
    );
  }
}

void MultiClientActor::on_archival_checked(size_t worker_index, bool is_archival) {
  LOG(DEBUG) << "LS #" << worker_index << " archival: " << is_archival;
  workers_[worker_index].is_archival = is_archival;
}

std::vector<size_t> MultiClientActor::select_workers(const RequestParameters& options) const {
  std::vector<size_t> result;
  if (!options.are_valid()) {
    LOG(WARNING) << "invalid request parameters";
    return result;
  }

  result.reserve(workers_.size());
  for (size_t i : std::views::iota(0u, workers_.size()) |
           std::views::filter([&](size_t i) { return workers_[i].is_alive; }) |
           std::views::filter([&](size_t i) { return options.archival == true ? workers_[i].is_archival : true; })) {
    result.push_back(i);
  }

  if (result.empty()) {
    return result;
  }

  switch (options.mode) {
    case RequestMode::Broadcast:
      return result;

    case RequestMode::Single: {
      if (options.lite_server_indexes.has_value()) {
        return std::find(result.begin(), result.end(), options.lite_server_indexes->front()) != result.end() ?
            std::vector<size_t>{options.lite_server_indexes.value().front()} :
            std::vector<size_t>{};
      }

      return std::vector<size_t>{result[get_random_index<size_t>(0, result.size() - 1)]};
    }

    case RequestMode::Multiple: {
      if (options.lite_server_indexes.has_value()) {
        std::vector<size_t> intersection_result;
        intersection_result.reserve(std::min<size_t>(options.clients_number.value(), result.size()));

        auto lite_server_indexes = options.lite_server_indexes.value();
        std::sort(result.begin(), result.end());
        std::sort(lite_server_indexes.begin(), lite_server_indexes.end());

        std::set_intersection(
            result.begin(),
            result.end(),
            lite_server_indexes.begin(),
            lite_server_indexes.end(),
            std::back_inserter(intersection_result)
        );
        result = std::move(intersection_result);
      }

      std::shuffle(result.begin(), result.end(), kRandomEngine);
      result.resize(std::min<size_t>(options.clients_number.value(), result.size()));
      return result;
    }
  }

  return result;
}


}  // namespace multiclient
