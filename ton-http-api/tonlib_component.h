#pragma once
#include "tonlib-multiclient/multi_client.h"
#include "userver/components/component_base.hpp"
#include "userver/dynamic_config/source.hpp"
#include "userver/utils/async.hpp"

namespace ton_http::core {
class TonlibComponent final : public userver::components::ComponentBase {
public:
  static constexpr std::string_view kName = "tonlib";

  TonlibComponent(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);

  [[nodiscard]] const multiclient::MultiClient& GetTonlib() const;
  ~TonlibComponent() final = default;

  template<typename T>
  td::Result<typename T::ReturnType> DoRequest(multiclient::Request<T> request) {
    auto task = userver::utils::Async(task_processor_, "tonlib_request", [this, R = std::move(request)] {
      return this->tonlib_.send_request(std::move(R));
    });
    return task.Get();
  };

  static userver::yaml_config::Schema GetStaticConfigSchema();
private:
  userver::dynamic_config::Source config_;
  multiclient::MultiClient tonlib_;
  userver::engine::TaskProcessor& task_processor_;
};
}
