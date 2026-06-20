#include "commands/XaddCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/StreamValue.hpp"
#include <cstddef>
#include <exception>
#include <string>
#include <utility>
#include <vector>

std::vector<RespValue>
XaddCommand::doExecute(const std::vector<RespValue> &args, RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto stream_entry_id = getStringValue(args.at(1));

  StreamValue::StreamEntry stream_entry;
  const std::size_t nargs = args.size();
  for (size_t i = 2; i < nargs; i += 2) {
    auto entry_key = getStringValue(args.at(i));
    auto entry_val = getStringValue(args.at(i + 1));
    stream_entry[entry_key] = entry_val;
  }

  try {
    auto saved_entry_id = store.addStreamEntry(store_key, stream_entry_id,
                                               std::move(stream_entry));
    result.emplace_back(RespBulkString(std::to_string(saved_entry_id.at(0)) +
                                       "-" +
                                       std::to_string(saved_entry_id.at(1))));
    return result;
  } catch (const std::exception &ex) {
    result.emplace_back(RespError(ex.what()));
    return result;
  }
}
