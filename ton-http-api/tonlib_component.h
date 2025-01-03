#pragma once
#include "tonlib-multiclient/multi_client.h"
#include "tonlib_postprocessor.h"
#include "tonlib_worker.h"
#include "userver/components/component_base.hpp"
#include "userver/dynamic_config/source.hpp"
#include "userver/utils/async.hpp"

namespace ton_http::core {
class TonlibComponent final : public userver::components::ComponentBase {
public:
  static constexpr std::string_view kName = "tonlib";

  TonlibComponent(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);
  ~TonlibComponent() final = default;

  // template<typename T>
  // td::Result<typename T::ReturnType> DoRequest(multiclient::Request<T> request) {
  //   auto task = userver::utils::Async(task_processor_, "tonlib_request", [this, R = std::move(request)] {
  //     return this->tonlib_.send_request(std::move(R));
  //   });
  //   return task.Get();
  // };
  // template<typename Func, typename... Args>
  // auto DoRequest(Func&& func, Args&&... args) -> decltype(auto) {
  //   auto task = userver::utils::Async(task_processor_, "tonlib_request", [this, func, ...capturedArgs = std::forward<Args>(args)] {
  //     return (this->worker_.get().*func)(std::forward<decltype(capturedArgs)>(capturedArgs)...);
  //   });
  //   return task.Get();
  // }

  template<typename Func, typename... Args>
  auto DoRequest(Func&& func, Args&&... args) -> decltype(auto) {
    auto task = userver::utils::Async(task_processor_, "tonlib_request", [this, func, ...args = std::forward<Args>(args)] {
      return (this->worker_.get()->*func)(args...);
    });
    return task.Get();
  }

  template<typename Func>
  auto DoRequest(Func&& func) -> decltype(auto) {
    auto task = userver::utils::Async(task_processor_, "tonlib_request", [this, func] {
      return (this->worker_.get()->*func)();
    });
    return task.Get();
  }

  template<typename Func, typename... Args>
  auto DoPostprocess(Func&& func, Args&&... args) -> decltype(auto) {
    return (this->postprocessor_.get()->*func)(std::forward<Args>(args)...);
  }

  template<typename Func>
  auto DoPostprocess(Func&& func) -> decltype(auto) {
    return (this->worker_.get()->*func)();
  }

  static userver::yaml_config::Schema GetStaticConfigSchema();
private:
  userver::dynamic_config::Source config_;
  std::unique_ptr<TonlibWorker> worker_;
  std::unique_ptr<TonlibPostProcessor> postprocessor_;
  userver::engine::TaskProcessor& task_processor_;
};
}
