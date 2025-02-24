#pragma once
#include "tonlib_component.h"
#include "userver/server/handlers/http_handler_base.hpp"

namespace ton_http::handlers {
struct TonlibApiRequest {
  std::string http_method;
  std::string ton_api_method;
  std::map<std::string, std::string> args;

  std::string GetArg(const std::string& name) const {
    const auto it = args.find(name);
    if (it == args.end()) { return ""; }
    return it->second;
  }
  void SetArg(const std::string& name, const std::string& value) {
    args.insert_or_assign(name, value);
  }
};

class ApiV2Handler final : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-api-v2";
  using HttpHandlerBase::HttpHandlerBase;
  std::string HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext& context) const override;
  ApiV2Handler(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);
private:
  ton_http::core::TonlibComponent& tonlib_component_;
  [[nodiscard]] core::TonlibWorkerResponse HandleTonlibRequest(const TonlibApiRequest& request) const;
};
}
