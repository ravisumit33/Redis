#include "commands/SetCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

std::vector<RespValue> SetCommand::doExecute(const std::vector<RespValue> &args,
                                             RedisStore &store) {
  std::vector<RespValue> result;

  const size_t nargs = args.size();
  auto key = getStringValue(args.at(0));
  auto value = getStringValue(args.at(1));
  std::optional<std::chrono::system_clock::time_point> expiry = std::nullopt;
  if (nargs == 4) {
    auto expiryArgName = getStringValue(args.at(2));
    std::string expiryArgLower = expiryArgName;
    std::ranges::transform(expiryArgLower, expiryArgLower.begin(),
                           [](unsigned char chr) { return std::tolower(chr); });
    if (expiryArgLower != "px") {
      result.emplace_back(RespError("Unsupported command arg"));
      return result;
    }
    try {
      auto expiry_time_str = getStringValue(args.at(3));
      auto expiryDelta = std::stoul(expiry_time_str);
      expiry = std::chrono::system_clock::now() +
               std::chrono::milliseconds(expiryDelta);
    } catch (const std::exception &ex) {
      auto expiry_time_str = getStringValue(args.at(3));
      std::cerr << "SET command: Invalid expiry time '" << expiry_time_str
                << "': " << ex.what() << '\n';
      result.emplace_back(RespError("Unsupported command arg"));
      return result;
    }
  }

  store.setString(key, value, expiry);
  std::cout << "Key: " << key << " set in store with value: " << value << '\n';
  result.emplace_back(RespString("OK"));
  return result;
}
