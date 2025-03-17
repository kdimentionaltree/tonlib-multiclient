#pragma once
#include <vector>

namespace multiclient {
class Session {
public:
  Session() = default;
  explicit Session(std::vector<size_t>&& active_workers) :
    active_workers_(std::move(active_workers)), start_time_(0) {
    start_time_ = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  }

  bool is_valid() const {
    return !active_workers_.empty();
  }
  const std::vector<size_t>& active_workers() const {
    return active_workers_;
  }
  void set_active_workers(std::vector<size_t>&& active_workers) {
    active_workers_ = std::move(active_workers);
  }
  double elapsed() const {
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    return double(now - start_time_) / 1000;
  }

  std::string to_string() const {
    std::stringstream res;
    res << ":";
    bool first = true;
    for (auto& worker : active_workers_) {
      if (!first) {
        res << ",";
      } else {
        first = false;
      }
      res << worker;
    }
    res << ":" << elapsed();
    return res.str();
  }
private:
  std::vector<size_t> active_workers_;
  std::uint64_t start_time_;
};
using SessionPtr = std::shared_ptr<Session>;
}
