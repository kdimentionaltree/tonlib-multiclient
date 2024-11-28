#include <unistd.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_set>
#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api.hpp"
#include "auto/tl/tonlib_api_json.h"
#include "multiclient/multi_client.h"
#include "multiclient/request.h"
#include "multiclient/response_callback.h"
#include "td/utils/JsonBuilder.h"
#include "td/utils/logging.h"
#include "tl/tl_json.h"
#include "tonlib/Logging.h"

struct Cb : public multiclient::ResponseCallback {
  Cb(std::unordered_set<uint64_t>& requests) : requests_(requests) {
  }
  ~Cb() override = default;

  void on_result(int64_t client_id, uint64_t req_id, tonlib_api::object_ptr<tonlib_api::Object> result) override {
    if (requests_.contains(req_id)) {
      auto serialized = td::json_encode<td::string>(td::ToJson(result));
      LOG(INFO) << "result | client_id: " << client_id << " req_id: " << req_id << " res_tl_id: " << result->get_id()
                << " << serialized << " << serialized;
      requests_.erase(req_id);
    }
  }

  void on_error(int64_t client_id, uint64_t req_id, tonlib_api::object_ptr<tonlib_api::error> error) override {
    if (requests_.contains(req_id)) {
      LOG(ERROR) << "error | client_id: " << client_id << " req_id: " << req_id << " code: " << error->code_
                 << " message: " << error->message_;
    }
  }

  std::unordered_set<uint64_t>& requests_;
};

int main(int argc, char* argv[]) {
  tonlib::Logging::set_verbosity_level(3);

  uint64_t request_id = 9999;
  std::unordered_set<uint64_t> requests{};

  multiclient::MultiClient client(
      multiclient::MultiClientConfig{
          .global_config_path = std::filesystem::path("/code/ton/ton-multiclient/global-config.json"),
          .key_store_root = std::filesystem::path("/code/ton/ton-multiclient/keystore"),
          .scheduler_threads = 6,
      },
      std::make_unique<Cb>(requests)
  );

  sleep(5);

  while (true) {
    sleep(5);

    auto req_id = request_id++;
    requests.insert(req_id);

    LOG(INFO) << "send request, id: " << req_id;
    client.send_callback_request(multiclient::RequestCallback{
        .parameters = {.mode = multiclient::RequestMode::Broadcast},
        .request_creator =
            []() {
              return ton::tonlib_api::make_object<ton::tonlib_api::getAccountState>(
                  ton::tonlib_api::make_object<ton::tonlib_api::accountAddress>(
                      "UQCD39VS5jcptHL8vMjEXrzGaRcCVYto7HUn4bpAOg8xqEBI"
                  )
              );
            },
        .request_id = req_id,
    });
  }

  return 0;
}
