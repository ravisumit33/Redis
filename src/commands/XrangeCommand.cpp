#include "commands/XrangeCommand.hpp"
#include "Connection.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"
#include "utils/genericUtils.hpp"

CommandRegistrar<XrangeCommand> XrangeCommand::registrar("XRANGE");

std::vector<std::unique_ptr<RespType>>
XrangeCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                           Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();

  auto store_val_ptr = RedisStore::instance().get(store_key);
  if (!store_val_ptr ||
      (*store_val_ptr)->getType() != RedisStoreValue::STREAM) {
    result.push_back(
        std::make_unique<RespError>("Key doesn't contain a stream"));
    return result;
  }

  auto stream_entry_id_start =
      static_cast<RespBulkString &>(*args.at(1)).getValue();
  auto stream_entry_id_end =
      static_cast<RespBulkString &>(*args.at(2)).getValue();

  auto stream_entries = RedisStore::instance().getStreamEntriesInRange(
      store_key, stream_entry_id_start, stream_entry_id_end);
  result.push_back(serializeStreamEntries(std::move(stream_entries)));
  return result;
}
