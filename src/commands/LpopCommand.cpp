#include "commands/LpopCommand.hpp"
#include "Connection.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"

CommandRegistrar<LpopCommand> LpopCommand::registrar("LPOP");

std::vector<std::unique_ptr<RespType>>
LpopCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  unsigned el_count = 1;
  if (args.size() == 2) {
    el_count =
        std::stoul(static_cast<RespBulkString &>(*args.at(1)).getValue());
  }
  auto [_, popped_elements] =
      RedisStore::instance().removeListElementsAtBegin(store_key, el_count);
  if (popped_elements.empty()) {
    result.push_back(std::make_unique<RespBulkString>());
    return result;
  }
  if (el_count == 1) {
    result.push_back(std::make_unique<RespBulkString>(popped_elements.at(0)));
    return result;
  }

  auto resp_array = std::make_unique<RespArray>();
  for (const auto &el : popped_elements) {
    resp_array->add(std::make_unique<RespBulkString>(el));
  }
  result.push_back(std::move(resp_array));
  return result;
}
