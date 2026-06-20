#include "commands/ZrankCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include <vector>

std::vector<RespValue>
ZrankCommand::doExecute(const std::vector<RespValue> &args, RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto member = getStringValue(args.at(1));

  try {
    auto rank = store.getSetMemberRank(store_key, member);
    result.emplace_back(RespInt(rank));
  } catch (...) {
    result.emplace_back(RespBulkString());
  }
  return result;
}
