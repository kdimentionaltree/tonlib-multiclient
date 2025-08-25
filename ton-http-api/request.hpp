#pragma once
#include <boost/iostreams/operations_fwd.hpp>


#include "tonlib_component.h"

namespace ton_http::handlers {
struct TonlibApiRequest {
  std::string http_method;
  std::string ton_api_method;
  std::map<std::string, std::vector<std::string>> args;
  bool is_debug_request{false};

  [[nodiscard]] std::string GetArg(const std::string& name) const {
    const auto it = args.find(name);
    if (it == args.end()) {
      return "";
    }
    if (it->second.empty()) {
      return "";
    }
    return it->second[0];
  }
  [[nodiscard]] const std::vector<std::string>& GetArgVector(const std::string& name) const {
    const auto it = args.find(name);
    if (it == args.end()) {
      return {};
    }
    return it->second;
  }
  void SetArg(const std::string& name, const std::string& value) {
    args.insert_or_assign(name, std::vector<std::string>{value});
  }
  void SetArgVector(const std::string& name, const std::vector<std::string>& values) {
    args.insert_or_assign(name, values);
  }

  friend bool operator==(const TonlibApiRequest& a, const TonlibApiRequest& b) {
    if (!(a.http_method == b.http_method && a.ton_api_method == b.ton_api_method)) {
      return false;
    }
    auto& lhs = a.args;
    auto& rhs = b.args;
    if (lhs.size() != rhs.size()) return false;
    for (auto& [k, v] : lhs) {
      auto it = rhs.find(k);
      if (it == rhs.end()) return false;

      auto v1 = v, v2 = it->second;
      std::ranges::sort(v1);
      std::ranges::sort(v2);
      if (v1 != v2) return false;
    }
    return true;
  }
};
}

namespace std {
template<>
struct hash<ton_http::handlers::TonlibApiRequest> {
  std::size_t operator()(const ton_http::handlers::TonlibApiRequest& request) const {
    std::stringstream ss;
    ss << request.http_method << "/" << request.ton_api_method;
    for (auto& [k, v] : request.args) {
      ss << "/" << k << ":";
      bool is_first = true;
      for (auto& i : v) {
        if (is_first) {
          is_first = false;
        } else {
          ss << ",";
        }
        ss << i;
      }
    }
    return std::hash<std::string>{}(ss.str());
  }
};
}
