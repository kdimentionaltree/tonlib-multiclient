#include <unistd.h>
#include "auto/tl/tonlib_api.h"
#include "tonlib-multiclient/multi_client.h"
#include "tonlib-multiclient/request.h"
#include "td/utils/logging.h"
#include "td/utils/misc.h"
#include "tonlib/Logging.h"

int main(int argc, char* argv[]) {
  tonlib::Logging::set_verbosity_level(3);

  multiclient::MultiClient client(multiclient::MultiClientConfig{
      .global_config_path = std::filesystem::path("/tmp/global-config.json"),
      .key_store_root = std::filesystem::path("/tmp/keystore"),
      .scheduler_threads = 6,
  });

  sleep(5);

  while (true) {
    sleep(5);
    LOG(INFO) << "send request";
    auto resp = client.send_request_function(multiclient::RequestFunction<ton::tonlib_api::raw_getAccountState>{
        .parameters = {.mode = multiclient::RequestMode::Broadcast},
        .request_creator =
            []() {
              return ton::tonlib_api::make_object<ton::tonlib_api::raw_getAccountState>(
                  ton::tonlib_api::make_object<ton::tonlib_api::accountAddress>(
                      "UQCD39VS5jcptHL8vMjEXrzGaRcCVYto7HUn4bpAOg8xqEBI"
                  )
              );
            },
    });

    if (resp.is_error()) {
      LOG(ERROR) << "resp: " << resp.error();
      continue;
    }

    auto res = resp.move_as_ok();
    LOG(INFO) << "balance: " << res->balance_ << " block: " << res->block_id_->seqno_
              << " code: " << td::hex_encode(res->code_) << " data: " << td::hex_encode(res->data_);
  }

  return 0;
}
