#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include "td/actor/PromiseFuture.h"

namespace multiclient {

template <typename T>
class PromiseSuccessAny {
private:
  struct ControlBlock {
    ControlBlock(td::Promise<T>&& p) : promise(std::move(p)) {
    }

    td::Promise<T> promise;
    std::atomic_int64_t pending_count{0};
    std::mutex mutex{};
  };

public:
  PromiseSuccessAny(td::Promise<T>&& promise) : control_block_(std::make_shared<ControlBlock>(std::move(promise))) {
  }

  td::Promise<T> get_promise() {
    control_block_->pending_count.fetch_add(1, std::memory_order_seq_cst);
    return [ctrl = control_block_](td::Result<T> res) {
      std::unique_lock<std::mutex> lock(ctrl->mutex);
      auto pending_count = ctrl->pending_count.fetch_sub(1, std::memory_order_relaxed);
      if (!res.is_ok()) {
        auto error = res.move_as_error();
        if (pending_count <= 1) {
          ctrl->promise.set_error(std::move(error));
        }
        return;
      }
      ctrl->promise.set_value(res.move_as_ok());
    };
  }

private:
  std::shared_ptr<ControlBlock> control_block_;
};

}  // namespace multiclient
