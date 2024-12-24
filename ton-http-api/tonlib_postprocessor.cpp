#include "tonlib_postprocessor.h"
#include "userver/formats/json.hpp"

namespace ton_http::core {
TonlibWorkerResponse TonlibPostProcessor::process_getAddressInformation(std::string address, td::Result<tonlib_api::raw_getAccountState::ReturnType> res) const {
  using namespace userver::formats::json;
  if (res.is_error()) {
    return TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }
  auto result = res.move_as_ok();
  ValueBuilder builder(FromString(td::json_encode<td::string>(td::ToJson(result))));

  auto r_std_address = block::StdAddress::parse(address);
  if (r_std_address.is_error()) {
    return TonlibWorkerResponse::from_error_string(r_std_address.move_as_error().to_string());
  }
  auto std_address = r_std_address.move_as_ok();
  return TonlibWorkerResponse{true, nullptr, ToString(builder.ExtractValue()), std::nullopt};
}
}
