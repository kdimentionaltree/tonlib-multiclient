#include "tonlib_component.h"

#include "tonlib_postprocessor.h"
#include "userver/components/component.hpp"
#include "userver/components/component_context.hpp"
#include "userver/dynamic_config/storage/component.hpp"
#include "userver/dynamic_config/value.hpp"
#include "userver/logging/log.hpp"
#include "userver/yaml_config/merge_schemas.hpp"


namespace ton_http::core {

TonlibComponent::TonlibComponent(
    const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context
) :
    userver::components::ComponentBase(config, context),
    config_(context.FindComponent<userver::components::DynamicConfig>().GetSource()),
    worker_(multiclient::MultiClientConfig{
        .global_config_path = config["global_config"].As<std::string>(),
        .key_store_root = config["keystore"].As<std::string>(),
        .scheduler_threads = config["threads"].As<std::size_t>()
    }),
    task_processor_(context.GetTaskProcessor(config["task_processor"].As<std::string>())) {
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
    task_processor:
        type: string
        description: task processor name
)");
}


}