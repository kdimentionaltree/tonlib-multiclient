#pragma once
#include <functional>

#include "request.hpp"
#include "userver/cache/expirable_lru_cache.hpp"
#include "userver/components/component_base.hpp"
#include "userver/components/component_context.hpp"
#include "userver/components/statistics_storage.hpp"
#include "userver/utils/statistics/entry.hpp"
#include "userver/yaml_config/schema.hpp"


namespace ton_http::cache {

namespace impl {
userver::yaml_config::Schema GetExpirableLruCacheStaticConfigSchema();
}

// clang-format on
template <typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ExpirableLruCacheComponent : public userver::components::ComponentBase {
public:
  using Cache = userver::cache::ExpirableLruCache<Key, Value, Hash, Equal>;

  ExpirableLruCacheComponent(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context)
  : ComponentBase(config, context), name_(userver::components::GetCurrentComponentName(context)),
  static_config_(config), cache_(std::make_shared<Cache>(static_config_.ways, static_config_.GetWaySize())) {
    cache_->SetMaxLifetime(static_config_.config.lifetime);
    cache_->SetBackgroundUpdate(static_config_.config.background_update);

    statistics_holder_ = context.FindComponent<userver::components::StatisticsStorage>().GetStorage().RegisterWriter(
    "cache", [this](userver::utils::statistics::Writer& writer) {
        writer = *cache_;
      }, {{"cache_name", name_}});
  }
  ~ExpirableLruCacheComponent() {
    statistics_holder_.Unregister();
  }

  static userver::yaml_config::Schema GetStaticConfigSchema() {
    return impl::GetExpirableLruCacheStaticConfigSchema();
  }

  std::optional<Value> Get(const Key& key) {
    return cache_->GetOptionalNoUpdate(key);
  }
  void Put(const Key& key, const Value& value) {
    return cache_->Put(key, value);
  }
private:
  const std::string name_;
  const userver::cache::LruCacheConfigStatic static_config_;
  const std::shared_ptr<Cache> cache_;

  userver::utils::statistics::Entry statistics_holder_;
};

class CacheApiV2Component final : public ExpirableLruCacheComponent<handlers::TonlibApiRequest, userver::formats::json::Value> {
public:
  static constexpr std::string_view kName = "cache-api-v2";
  CacheApiV2Component(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context)
    : ExpirableLruCacheComponent(config, context) {};
};

class ConstCacheApiV2Component final : public ExpirableLruCacheComponent<handlers::TonlibApiRequest, userver::formats::json::Value> {
public:
  static constexpr std::string_view kName = "const-cache-api-v2";
  ConstCacheApiV2Component(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context)
    : ExpirableLruCacheComponent(config, context) {};
};
// clang-format off
}
