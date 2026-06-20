#include "commands/ZremCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
ZremCommand::doExecute(const std::vector<RespValue> &args,
                       RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto member = getStringValue(args.at(1));

  std::size_t removed_cnt = 0;
  try {
    removed_cnt =
        store.removeMemberFromSet(store_key, member);
  } catch (...) {
  }

  result.emplace_back(RespInt(static_cast<int64_t>(removed_cnt)));
  return result;
}

std::vector<RespValue>
ZremCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
ZremCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
