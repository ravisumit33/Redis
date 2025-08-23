#include "commands/ZrankCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include <memory>

CommandRegistrar<ZrankCommand> ZrankCommand::registrar("ZRANK");

std::vector<std::unique_ptr<RespType>>
ZrankCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto member = static_cast<RespBulkString &>(*args.at(1)).getValue();

  try {
    auto rank = RedisStore::instance().getSetMemberRank(store_key, member);
    result.push_back(std::make_unique<RespInt>(rank));
  } catch (...) {
    result.push_back(std::make_unique<RespBulkString>());
  }
  return result;
}
