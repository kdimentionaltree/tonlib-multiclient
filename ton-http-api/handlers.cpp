#include "handlers.hpp"
#include "userver/formats/json.hpp"

namespace ton_http::handlers {

std::string HelloHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request, userver::server::request::RequestContext& context
) const {
  request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
  auto& name = request.GetArg("name");

  auto result = userver::formats::json::ValueBuilder();
  result["ok"] = true;
  result["result"] = "Hello!";
  return userver::formats::json::ToString(result.ExtractValue());
}
}  // namespace ton_http::handlers
