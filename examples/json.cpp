#include <unistd.h>
#include "multiclient/multi_client.h"
#include "multiclient/request.h"
#include "td/utils/logging.h"

int main(int argc, char* argv[]) {
  multiclient::MultiClient client(multiclient::MultiClientConfig{
      .global_config_path = std::filesystem::path("/code/ton/ton-multiclient/global-config.json"),
      .key_store_root = std::filesystem::path("/code/ton/ton-multiclient/keystore"),
      .scheduler_threads = 6,
  });

  sleep(5);

  while (true) {
    sleep(5);
    LOG(INFO) << "send request";
    auto resp = client.send_request_json(multiclient::RequestJson{
        .parameters = {.mode = multiclient::RequestMode::Broadcast},
        .request =
            R"({"@type":"getAccountState","account_address":{"@type":"accountAddress","account_address":"UQCD39VS5jcptHL8vMjEXrzGaRcCVYto7HUn4bpAOg8xqEBI"}})",
    });

    if (resp.is_error()) {
      LOG(ERROR) << "resp: " << resp.error();
      continue;
    }

    LOG(INFO) << "result: " << resp.move_as_ok();
  }

  return 0;
}
