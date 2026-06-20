#include "commands/ZrankCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
ZrankCommand::doExecute(const std::vector<RespValue> &args,
                        RedisStore &store) {
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

std::vector<RespValue>
ZrankCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
ZrankCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
