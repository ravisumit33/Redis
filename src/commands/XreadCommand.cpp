#include "commands/XreadCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "utils/genericUtils.hpp"
#include <optional>
#include <vector>

std::vector<RespValue>
XreadCommand::doExecute(const std::vector<RespValue> &args,
                        RedisStore &store) {
  std::vector<RespValue> result;
  std::size_t arg_idx = 0;
  std::optional<uint64_t> timeout_ms;
  auto first_arg = getStringValue(args.at(arg_idx));
  ++arg_idx;
  if (first_arg == "block") {
    timeout_ms = std::stoull(getStringValue(args.at(arg_idx)));
    ++arg_idx;
    auto third_arg = getStringValue(args.at(arg_idx));
    ++arg_idx;

    if (third_arg != "streams") {
      result.emplace_back(RespError("Unsupported read_type arg: " + third_arg));
      return result;
    }
  } else if (first_arg != "streams") {
    result.emplace_back(RespError("Unsupported read_type arg: " + first_arg));
    return result;
  }

  std::size_t pairs_count = (args.size() - arg_idx) / 2;
  std::vector<std::string> store_keys;
  std::vector<std::string> entry_ids_start;
  for (std::size_t i = 0; i < pairs_count; ++i) {
    auto store_key = getStringValue(args.at(arg_idx + i));
    auto stream_entry_id_start =
        getStringValue(args.at(arg_idx + i + pairs_count));
    store_keys.emplace_back(store_key);
    entry_ids_start.emplace_back(stream_entry_id_start);
  }
  auto [timed_out, stream_entries_map] =
      store.getStreamEntriesAfterAny(
          store_keys, entry_ids_start, timeout_ms);

  if (timed_out) {
    result.emplace_back(RespArray::null());
    return result;
  }

  RespArray resp_array;

  for (const auto &store_key : store_keys) {
    if (stream_entries_map.contains(store_key)) {
      auto stream_entries = stream_entries_map[store_key];
      RespArray key_array;
      key_array.add(RespBulkString(store_key))
          .add(serializeStreamEntries(stream_entries));
      resp_array.add(std::move(key_array));
    }
  }

  result.emplace_back(std::move(resp_array));
  return result;
}

std::vector<RespValue>
XreadCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
XreadCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
