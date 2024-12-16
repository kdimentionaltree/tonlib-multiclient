#pragma once

#include <cstdint>
#include "auto/tl/tonlib_api.h"

namespace multiclient {

class ResponseCallback {
public:
  virtual void on_result(
      int64_t client_id, uint64_t id, ton::tonlib_api::object_ptr<ton::tonlib_api::Object> result
  ) = 0;
  virtual void on_error(int64_t client_id, uint64_t id, ton::tonlib_api::object_ptr<ton::tonlib_api::error> error) = 0;
  virtual ~ResponseCallback() = default;
};

}  // namespace multiclient
