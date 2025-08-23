#include "commands/XaddCommand.hpp"
#include "Connection.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include <exception>
#include <string>

CommandRegistrar<XaddCommand> XaddCommand::registrar("XADD");

std::vector<std::unique_ptr<RespType>>
XaddCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto stream_entry_id = static_cast<RespBulkString &>(*args.at(1)).getValue();

  StreamValue::StreamEntry stream_entry;
  std::size_t nargs = args.size();
  for (int i = 2; i < nargs; i += 2) {
    auto entry_key = static_cast<RespBulkString &>(*args.at(i)).getValue();
    auto entry_val = static_cast<RespBulkString &>(*args.at(i + 1)).getValue();
    stream_entry[entry_key] = entry_val;
  }

  try {
    auto saved_entry_id = RedisStore::instance().addStreamEntry(
        store_key, stream_entry_id, std::move(stream_entry));
    result.push_back(std::make_unique<RespBulkString>(
        std::to_string(saved_entry_id.at(0)) + "-" +
        std::to_string(saved_entry_id.at(1))));
    return result;
  } catch (const std::exception &ex) {
    result.push_back(std::make_unique<RespError>(ex.what()));
    return result;
  }
}
