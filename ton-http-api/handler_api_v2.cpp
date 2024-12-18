#include "handler_api_v2.h"

#include "tl/tl_json.h"
#include "auto/tl/tonlib_api.h"
#include "auto/tl/tonlib_api_json.h"
#include "td/utils/JsonBuilder.h"
#include "userver/components/component_context.hpp"
#include "userver/http/common_headers.hpp"
#include "openapi/openapi_page.hpp"

namespace ton_http::handlers {
std::string ApiV2Handler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request, userver::server::request::RequestContext& context
) const {
  auto& ton_api_method = request.GetPathArg("ton_api_method");
  LOG_WARNING() << "Got request " << ton_api_method;

  if ((ton_api_method == "index.html") || ton_api_method.empty()) {
    request.GetHttpResponse().SetHeader(userver::http::headers::kContentType, "text/html; charset=utf-8");
    return openapi::GetOpenApiPage();
  }
  if (ton_api_method == "openapi.json") {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
    return openapi::GetOpenApiJson();
  }
  // call method
  request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
  core::TonlibWorkerResponse response{false, nullptr, td::Status::Error("not implemented")};
  if (ton_api_method == "getMasterchainInfo") {
    response = tonlib_component_.DoRequest(&core::TonlibWorker::getMasterchainInfo);
  }

  auto result = userver::formats::json::ValueBuilder();
  result["ok"] = response.is_ok;
  if (response.is_ok) {
    result["result"] = userver::formats::json::FromString(td::json_encode<td::string>(td::ToJson(response.result)));
  } else {
    result["error"] = response.error->to_string();
  }
  return userver::formats::json::ToString(result.ExtractValue());
}
ApiV2Handler::ApiV2Handler(
    const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context
) :
    HttpHandlerBase(config, context), tonlib_component_(context.FindComponent<ton_http::core::TonlibComponent>()) {
}

}  // namespace ton_http::handlers
