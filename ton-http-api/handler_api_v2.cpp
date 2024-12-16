#include "handler_api_v2.h"

#include "tl/tl_json.h"
#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api_json.h"
#include "td/utils/JsonBuilder.h"
#include "tonlib-multiclient/request.h"
#include "userver/components/component_context.hpp"

namespace ton_http::handlers {
std::string ApiV2Handler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request, userver::server::request::RequestContext& context
) const {
  request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
  auto& ton_api_method = request.GetPathArg("ton_api_method");
  LOG_WARNING() << "Got request " << ton_api_method;

  // call method
  td::StringBuilder result_sb;
  bool is_ok = false;
  if (ton_api_method == "getMasterchainInfo") {
    auto resp = tonlib_component_.DoRequest(multiclient::Request<ton::tonlib_api::blocks_getMasterchainInfo>{
      .parameters = {.mode = multiclient::RequestMode::Multiple, .clients_number = 1},
      .request_creator = [] {
        return ton::tonlib_api::blocks_getMasterchainInfo();
      },
    });
    if (resp.is_error()) {
      result_sb << resp.move_as_error();
    } else {
      const auto res = resp.move_as_ok();
      auto str = td::json_encode<td::string>(td::ToJson(res));
      result_sb << str;
      is_ok = true;
    }
  }
  else {
    result_sb << "not implemented";
  }

  auto result = userver::formats::json::ValueBuilder();
  result["ok"] = is_ok;
  result[(is_ok? "result" : "error")] = userver::formats::json::FromString(result_sb.as_cslice().str());
  return userver::formats::json::ToString(result.ExtractValue());
}
ApiV2Handler::ApiV2Handler(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context)
  : HttpHandlerBase(config, context), tonlib_component_(context.FindComponent<ton_http::core::TonlibComponent>())
{}

}