#pragma once

#include "tonlib_worker.h"

namespace ton_http::core {
class TonlibPostProcessor {
public:
  TonlibWorkerResponse process_getAddressInformation(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res, multiclient::SessionPtr&& session = nullptr) const;
  TonlibWorkerResponse process_getWalletInformation(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res, multiclient::SessionPtr&& session = nullptr) const;
  TonlibWorkerResponse process_getAddressBalance(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res, multiclient::SessionPtr&& session = nullptr) const;
  TonlibWorkerResponse process_getAddressState(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res, multiclient::SessionPtr&& session = nullptr) const;
  TonlibWorkerResponse process_getBlockTransactions(td::Result<tonlib_api::blocks_getTransactions::ReturnType>&& res, multiclient::SessionPtr&& session = nullptr) const;
  TonlibWorkerResponse process_getBlockTransactionsExt(td::Result<tonlib_api::blocks_getTransactionsExt::ReturnType>&& res, multiclient::SessionPtr&& session = nullptr) const;
  TonlibWorkerResponse process_getTransactions(td::Result<tonlib_api::raw_getTransactionsV2::ReturnType>&& res, bool v2_schema = true, bool unwrap_single_transaction = false, multiclient::SessionPtr&& session = nullptr) const;
  TonlibWorkerResponse process_runGetMethod(td::Result<RunGetMethodResult>&& res, multiclient::SessionPtr&& session = nullptr) const;
};
}
