#include "commands/XaddCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "redis_store/values/StreamValue.hpp"
#include <exception>
#include <string>

std::vector<RespValue>
XaddCommand::doExecute(const std::vector<RespValue> &args,
                       RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto stream_entry_id = getStringValue(args.at(1));

  StreamValue::StreamEntry stream_entry;
  std::size_t nargs = args.size();
  for (size_t i = 2; i < nargs; i += 2) {
    auto entry_key = getStringValue(args.at(i));
    auto entry_val = getStringValue(args.at(i + 1));
    stream_entry[entry_key] = entry_val;
  }

  try {
    auto saved_entry_id = store.addStreamEntry(
        store_key, stream_entry_id, std::move(stream_entry));
    result.emplace_back(RespBulkString(std::to_string(saved_entry_id.at(0)) +
                                       "-" +
                                       std::to_string(saved_entry_id.at(1))));
    return result;
  } catch (const std::exception &ex) {
    result.emplace_back(RespError(ex.what()));
    return result;
  }
}

std::vector<RespValue>
XaddCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
XaddCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
