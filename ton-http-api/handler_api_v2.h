#pragma once
#include "tonlib_component.h"
#include "userver/server/handlers/http_handler_base.hpp"

namespace ton_http::handlers {
class ApiV2Handler final : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-api-v2";
  using HttpHandlerBase::HttpHandlerBase;
  std::string HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext& context) const override;
  ApiV2Handler(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);
private:
  ton_http::core::TonlibComponent& tonlib_component_;
};
}
