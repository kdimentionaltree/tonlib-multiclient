#pragma once

#include "tonlib_worker.h"

namespace ton_http::core {
class TonlibPostProcessor {
public:
  TonlibWorkerResponse process_getAddressInformation(std::string address, td::Result<tonlib_api::raw_getAccountState::ReturnType>) const;
};
}
