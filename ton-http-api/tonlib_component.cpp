#include "tonlib_component.h"

#include "core/src/curl-ev/form.hpp"
#include "tonlib_postprocessor.h"
#include "userver/clients/http/component.hpp"
#include "userver/components/component.hpp"
#include "userver/components/component_context.hpp"
#include "userver/dynamic_config/storage/component.hpp"
#include "userver/dynamic_config/value.hpp"
#include "userver/formats/json/value_builder.hpp"
#include "userver/logging/component.hpp"
#include "userver/logging/log.hpp"
#include "userver/yaml_config/merge_schemas.hpp"


namespace ton_http::core {

TonlibComponent::TonlibComponent(
    const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context
) :
    userver::components::ComponentBase(config, context),
    config_(context.FindComponent<userver::components::DynamicConfig>().GetSource()),
    worker_(
        std::make_unique<TonlibWorker>(multiclient::MultiClientConfig{
            .global_config_path = config["global_config"].As<std::string>(),
            .key_store_root = config["keystore"].As<std::string>(),
            .blockchain_name = "",
            .reset_key_store = false,
            .scheduler_threads = config["threads"].As<std::size_t>(),
        })
    ),
    task_processor_(context.GetTaskProcessor(config["task_processor"].As<std::string>())),
    external_message_endpoints_(config["external_message_endpoints"].As<std::vector<std::string>>(std::vector<std::string>{})),
    logger_(context.FindComponent<userver::components::Logging>().GetLogger("api-v2")),
    http_client_(context.FindComponent<userver::components::HttpClient>().GetHttpClient()){
}

bool TonlibComponent::SendBocToExternalRequest(std::string boc_b64) {
  if (external_message_endpoints_.empty()) {
    return true;
  }

  userver::formats::json::ValueBuilder builder;
  builder["boc"] = boc_b64;
  auto body = ToString(builder.ExtractValue());

  auto success = true;
  for (const auto& endpoint : external_message_endpoints_) {
    auto request = http_client_.CreateRequest().post(endpoint, body) \
    .headers({{"Content-Type", "application/json"}}) \
    .timeout(std::chrono::seconds(1)).perform();
    if (!request->IsOk()) {
      LOG_WARNING_TO(*logger_) << "Failed to send BOC to external endpoint: " << endpoint
                               << " Error: " << request->body_view();
      success = false;
    }
  }
  return success;
}
userver::yaml_config::Schema TonlibComponent::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
type: object
description: tonlib component config
additionalProperties: false
properties:
    global_config:
        type: string
        description: path to TON network config
    keystore:
        type: string
        description: path to Tonlib keystore
    threads:
        type: integer
        description: number of Tonlib threads
    external_message_endpoints:
        type: array
        description: list of external endpoints for sendBoc method
        defaultDescription: <empty>
        items:
          type: string
          description: external endpoint url
    task_processor:
        type: string
        description: task processor name
)");
}


}