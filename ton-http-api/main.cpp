#include "handler_api_v2.h"
#include "userver/components/minimal_server_component_list.hpp"
#include "userver/server/handlers/server_monitor.hpp"
#include "userver/utils/daemon_run.hpp"

#include "handlers.hpp"
#include "tonlib/Logging.h"
#include "tonlib_component.h"

int main(int argc, char* argv[]) {
  tonlib::Logging::set_verbosity_level(3);

  auto component_list = userver::components::MinimalServerComponentList();
  component_list.Append<userver::server::handlers::ServerMonitor>();
  component_list.Append<ton_http::core::TonlibComponent>();
  component_list.Append<ton_http::handlers::ApiV2Handler>();
  return userver::utils::DaemonMain(argc, argv, component_list);
}
