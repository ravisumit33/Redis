#include "commands/XrangeCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/StreamValue.hpp"
#include "utils/genericUtils.hpp"
#include <utility>
#include <variant>
#include <vector>

std::vector<RespValue>
XrangeCommand::doExecute(const std::vector<RespValue> &args,
                         RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));

  auto store_val_ptr = store.get(store_key);
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

  auto stream_entries = store.getStreamEntriesInRange(
      store_key, stream_entry_id_start, stream_entry_id_end);
  result.emplace_back(serializeStreamEntries(std::move(stream_entries)));
  return result;
}
