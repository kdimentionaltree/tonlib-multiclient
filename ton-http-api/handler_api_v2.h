#pragma once
#include "request.hpp"
#include "cache.hpp"
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
  core::TonlibComponent& tonlib_component_;
  cache::CacheApiV2Component& cache_component_;
  userver::logging::LoggerPtr logger_;
  [[nodiscard]] core::TonlibWorkerResponse HandleTonlibRequest(const TonlibApiRequest& request) const;
  [[nodiscard]] bool is_log_required(const TonlibApiRequest& request, const core::TonlibWorkerResponse& response) const;
  [[nodiscard]] userver::formats::json::Value build_json_response(const core::TonlibWorkerResponse& res) const;
  [[nodiscard]] std::vector<std::string> parse_request_body_item(const userver::formats::json::Value& value, int parse_array_depth=0) const;
  void log_request(
      const userver::server::http::HttpRequest& request,
      const TonlibApiRequest& req,
      const core::TonlibWorkerResponse& tonlib_response,
      const std::string& response_body
  ) const;
};
}
