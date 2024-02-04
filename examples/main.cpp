#include <unistd.h>
#include "auto/tl/tonlib_api.h"
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
    auto resp =
        client.send_request<ton::tonlib_api::getAccountState>(multiclient::Request<ton::tonlib_api::getAccountState>{
            .parameters =
                {
                    .mode = multiclient::RequestMode::Broadcast,
                },
            .request_creator = []() -> ton::tonlib_api::getAccountState {
              return ton::tonlib_api::getAccountState(ton::tonlib_api::make_object<ton::tonlib_api::accountAddress>(
                  "UQCD39VS5jcptHL8vMjEXrzGaRcCVYto7HUn4bpAOg8xqEBI"
              ));
            },
        });

    if (resp.is_error()) {
      LOG(ERROR) << "resp: " << resp.error();
      continue;
    }

    auto res = resp.move_as_ok();
    LOG(INFO) << "balance: " << res->balance_ << " block: " << res->block_id_->seqno_;
  }

  return 0;
}
