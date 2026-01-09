#include "commands/ZremCommand.hpp"
#include "AppContext.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
ZremCommand::doExecute(const std::vector<RespValue> &args,
                       AppContext &context) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto member = getStringValue(args.at(1));

  std::size_t removed_cnt = 0;
  try {
    removed_cnt =
        context.getRedisStore().removeMemberFromSet(store_key, member);
  } catch (...) {
  }

  result.emplace_back(RespInt(static_cast<int64_t>(removed_cnt)));
  return result;
}

std::vector<RespValue>
ZremCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext());
}

std::vector<RespValue>
ZremCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext());
}
