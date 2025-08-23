#include "commands/LpushCommand.hpp"
#include "Connection.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include <vector>

CommandRegistrar<LpushCommand> LpushCommand::registrar("LPUSH");

std::vector<std::unique_ptr<RespType>>
LpushCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  std::vector<std::string> elements;
  std::transform(args.begin() + 1, args.end(), std::back_inserter(elements),
                 [](const auto &arg) {
                   return static_cast<RespBulkString &>(*arg).getValue();
                 });
  std::reverse(elements.begin(), elements.end());
  auto list_size =
      RedisStore::instance().addListElementsAtBegin(store_key, elements);
  result.push_back(std::make_unique<RespInt>(list_size));
  return result;
}
