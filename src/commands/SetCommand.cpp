#include "commands/SetCommand.hpp"
#include "Connection.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>

CommandRegistrar<SetCommand> SetCommand::registrar("SET");

std::vector<std::unique_ptr<RespType>>
SetCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                        Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;

  size_t nargs = args.size();
  auto key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto value = static_cast<RespBulkString &>(*args.at(1)).getValue();
  std::optional<std::chrono::system_clock::time_point> expiry = std::nullopt;
  if (nargs == 4) {
    auto expiryArgName = static_cast<RespBulkString &>(*args.at(2)).getValue();
    if (expiryArgName != "px") {
      result.push_back(std::make_unique<RespError>("Unsupported command arg"));
      return result;
    }
    try {
      auto expiry_time_str =
          static_cast<RespBulkString &>(*args.at(3)).getValue();
      auto expiryDelta = std::stoul(expiry_time_str);
      expiry = std::chrono::system_clock::now() +
               std::chrono::milliseconds(expiryDelta);
    } catch (const std::exception &ex) {
      auto expiry_time_str =
          static_cast<RespBulkString &>(*args.at(3)).getValue();
      std::cerr << "SET command: Invalid expiry time '" << expiry_time_str
                << "': " << ex.what() << std::endl;
      result.push_back(std::make_unique<RespError>("Unsupported command arg"));
      return result;
    }
  }

  RedisStore::instance().setString(key, value, expiry);
  std::cout << "Key: " << key << " set in store with value: " << value
            << std::endl;
  result.push_back(std::make_unique<RespString>("OK"));
  return result;
}
