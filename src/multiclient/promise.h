#pragma once

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
    int64_t pending_count = 0;
    std::mutex mutex;
  };

public:
  PromiseSuccessAny(td::Promise<T>&& promise) : control_block_(std::make_shared<ControlBlock>(std::move(promise))) {
  }

  td::Promise<T> get_promise() {
    control_block_->pending_count += 1;
    return [ctrl = control_block_](td::Result<T> res) {
      std::unique_lock<std::mutex> lock(ctrl->mutex);
      ctrl->pending_count -= 1;
      if (!res.is_ok()) {
        if (ctrl->pending_count <= 0) {
          ctrl->promise.set_error(td::Status::Error("All promises failed"));
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
