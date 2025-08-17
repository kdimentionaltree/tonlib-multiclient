#pragma once

namespace ton_http::utils {
template <typename IntType>
std::optional<IntType> stringToInt(const std::string& str) {
  static_assert(std::is_integral<IntType>::value, "Template parameter must be an integral type");
  try {
    if constexpr (std::is_same<IntType, int>::value) {
      return static_cast<IntType>(std::stoi(str));
    } else if constexpr (std::is_same<IntType, long>::value) {
      return static_cast<IntType>(std::stol(str));
    } else if constexpr (std::is_same<IntType, long long>::value) {
      return static_cast<IntType>(std::stoll(str));
    } else if constexpr (std::is_same<IntType, unsigned long>::value) {
      return static_cast<IntType>(std::stoul(str));
    } else if constexpr (std::is_same<IntType, unsigned long long>::value) {
      return static_cast<IntType>(std::stoull(str));
    } else if constexpr (std::is_same<IntType, unsigned int>::value) {
      return static_cast<IntType>(std::stoul(str));
    } else if constexpr (std::is_same<IntType, short>::value) {
      return static_cast<IntType>(std::stoi(str));
    } else if constexpr (std::is_same<IntType, unsigned short>::value) {
      return static_cast<IntType>(std::stoul(str));
    } else {
      throw std::invalid_argument("Unsupported integer type");
    }
  } catch (...) {
    return std::nullopt;
  }
}

inline std::optional<bool> stringToBool(std::string str) {
  if (str.empty()) {
    return std::nullopt;
  }
  std::ranges::transform(str, str.begin(), ::tolower);
  if (str == "y" || str == "yes" || str == "t" || str == "true" || str == "on" || str == "1") {
    return true;
  }
  if (str == "n" || str == "no" || str == "f" || str == "false" || str == "off" || str == "0") {
    return false;
  } return std::nullopt;
}

inline std::optional<std::string> stringToHash(const std::string& str) {
  if (str.empty()) {
    return str;
  }

  if (str.length() == 44) {
    if (auto res = td::base64_decode(str); res.is_ok()) {
      return res.move_as_ok();
    }
    if (auto res = td::base64url_decode(str); res.is_ok()) {
      return res.move_as_ok();
    }
  } else if (str.length() == 43) {
    if (auto res = td::base64url_decode(str); res.is_ok()) {
      return res.move_as_ok();
    }
  } else if (str.length() == 64) {
    if (auto res = td::hex_decode(str); res.is_ok()) {
      return res.move_as_ok();
    }
  }
  return std::nullopt;
}
}
