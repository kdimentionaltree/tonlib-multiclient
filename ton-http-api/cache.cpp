#include "cache.hpp"

#include "userver/cache/lru_cache_component_base.hpp"
#include "userver/components/component_context.hpp"
#include "userver/components/statistics_storage.hpp"
#include "userver/yaml_config/merge_schemas.hpp"


userver::yaml_config::Schema ton_http::cache::impl::GetExpirableLruCacheStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
type: object
description: Expirable LRU-cache component
additionalProperties: false
properties:
    size:
        type: integer
        description: max amount of items to store in cache
    ways:
        type: integer
        description: number of ways for associative cache
    lifetime:
        type: string
        description: TTL for cache entries (0 is unlimited)
        defaultDescription: 0
    background-update:
        type: boolean
        description: enables asynchronous updates for expiring values
        defaultDescription: false
    config-settings:
        type: boolean
        description: enables dynamic reconfiguration with CacheConfigSet
        defaultDescription: true
)");
}