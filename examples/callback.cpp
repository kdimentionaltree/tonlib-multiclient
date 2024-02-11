#include <unistd.h>
#include <cstdint>
#include <mutex>
#include <unordered_set>
#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api.hpp"
#include "auto/tl/tonlib_api_json.h"
#include "multiclient/multi_client.h"
#include "multiclient/request.h"
#include "td/utils/JsonBuilder.h"
#include "td/utils/logging.h"
#include "td/utils/unique_ptr.h"
#include "tl/tl_json.h"
#include "tonlib/TonlibCallback.h"

struct Cb : public tonlib::TonlibCallback {
  Cb(std::unordered_set<uint64_t>& requests) : requests_(requests) {
  }

  void on_result(uint64_t id, tonlib_api::object_ptr<tonlib_api::Object> result) override {
    if (requests_.contains(id)) {
      auto serialized = td::json_encode<td::string>(td::ToJson(result));
      LOG(INFO) << "result id: " << result->get_id() << " serialized: " << serialized;
      requests_.erase(id);
    }
  }

  void on_error(uint64_t id, tonlib_api::object_ptr<tonlib_api::error> error) override {
    if (requests_.contains(id)) {
      LOG(ERROR) << "error | code: " << error->code_ << " message: " << error->message_;
    }
  }

  std::unordered_set<uint64_t>& requests_;
};

int main(int argc, char* argv[]) {
  uint64_t request_id = 9999;
  std::unordered_set<uint64_t> requests{};

  multiclient::MultiClient client(
      multiclient::MultiClientConfig{
          .global_config_path = std::filesystem::path("/code/ton/ton-multiclient/local-config.json"),
          .key_store_root = std::filesystem::path("/code/ton/ton-multiclient/keystore"),
          .scheduler_threads = 6,
      },
      td::make_unique<Cb>(requests)
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
