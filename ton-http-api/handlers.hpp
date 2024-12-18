#pragma once

#include "userver/components/component_list.hpp"
#include "userver/server/handlers/http_handler_base.hpp"
#include "userver/server/handlers/http_handler_base.hpp"

// #include "userver/utest/using_namespace_userver.hpp"

namespace ton_http::handlers {

class HelloHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-hello-sample";

    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequestThrow(const userver::server::http::HttpRequest &request, userver::server::request::RequestContext &context) const override;
};
}
