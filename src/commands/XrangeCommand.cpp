#include "commands/XrangeCommand.hpp"
#include "AppContext.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "redis_store/values/StreamValue.hpp"
#include "utils/genericUtils.hpp"

std::vector<RespValue>
XrangeCommand::doExecute(const std::vector<RespValue> &args,
                         AppContext &context) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));

  auto store_val_ptr = context.getRedisStore().get(store_key);
  if (!store_val_ptr) {
    result.emplace_back(RespError("Key doesn't contain a stream"));
    return result;
  }

  auto *stream_val = std::get_if<StreamValue>(&store_val_ptr.value());
  if (stream_val == nullptr) {
    result.emplace_back(RespError("Key doesn't contain a stream"));
    return result;
  }

  auto stream_entry_id_start = getStringValue(args.at(1));
  auto stream_entry_id_end = getStringValue(args.at(2));

  auto stream_entries = context.getRedisStore().getStreamEntriesInRange(
      store_key, stream_entry_id_start, stream_entry_id_end);
  result.emplace_back(serializeStreamEntries(std::move(stream_entries)));
  return result;
}

std::vector<RespValue>
XrangeCommand::executeOnImpl(const std::vector<RespValue> &args,
                             ClientConnection &connection) {
  return doExecute(args, connection.getContext());
}

std::vector<RespValue>
XrangeCommand::executeOnImpl(const std::vector<RespValue> &args,
                             ServerConnection &connection) {
  return doExecute(args, connection.getContext());
}
