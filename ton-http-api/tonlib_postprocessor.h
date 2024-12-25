#pragma once

#include "tonlib_worker.h"

namespace ton_http::core {
class TonlibPostProcessor {
public:
  TonlibWorkerResponse process_getAddressInformation(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res) const;
  TonlibWorkerResponse process_getWalletInformation(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res) const;
  TonlibWorkerResponse process_getAddressBalance(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res) const;
  TonlibWorkerResponse process_getAddressState(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res) const;
};
}
