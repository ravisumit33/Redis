#include "commands/BlpopCommand.hpp"
#include "Connection.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"
#include <iostream>
#include <optional>

CommandRegistrar<BlpopCommand> BlpopCommand::registrar("BLPOP");

std::vector<std::unique_ptr<RespType>>
BlpopCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto timeout_s =
      std::stod(static_cast<RespBulkString &>(*args.at(1)).getValue());
  std::cout << "Store key: " << store_key << ", Timeout: " << timeout_s
            << std::endl;
  auto [timed_out, popped_elements] =
      RedisStore::instance().removeListElementsAtBegin(store_key, 1, timeout_s);
  if (timed_out) {
    result.push_back(std::make_unique<RespBulkString>());
    return result;
  }
  auto resp_array = std::make_unique<RespArray>();
  resp_array->add(std::make_unique<RespBulkString>(store_key))
      ->add(std::make_unique<RespBulkString>(popped_elements.at(0)));

  result.push_back(std::move(resp_array));
  return result;
}
