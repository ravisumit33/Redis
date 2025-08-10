#include "commands/XreadCommand.hpp"
#include "Connection.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"
#include "utils/genericUtils.hpp"
#include <optional>
#include <vector>

CommandRegistrar<XreadCommand> XreadCommand::registrar("XREAD");

std::vector<std::unique_ptr<RespType>>
XreadCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  std::size_t arg_idx = 0;
  std::optional<uint64_t> timeout_ms;
  auto first_arg = static_cast<RespBulkString &>(*args.at(arg_idx)).getValue();
  ++arg_idx;
  if (first_arg == "block") {
    timeout_ms = std::stoull(
        static_cast<RespBulkString &>(*args.at(arg_idx)).getValue());
    ++arg_idx;
    auto third_arg =
        static_cast<RespBulkString &>(*args.at(arg_idx)).getValue();
    ++arg_idx;

    if (third_arg != "streams") {
      result.push_back(std::make_unique<RespError>(
          "Unsupported read_type arg: " + third_arg));
      return result;
    }
  } else if (first_arg != "streams") {
    result.push_back(
        std::make_unique<RespError>("Unsupported read_type arg: " + first_arg));
    return result;
  }

  std::size_t pairs_count = (args.size() - arg_idx) / 2;
  std::vector<std::string> store_keys, entry_ids_start;
  for (std::size_t i = 0; i < pairs_count; ++i) {
    auto store_key =
        static_cast<RespBulkString &>(*args.at(arg_idx + i)).getValue();
    auto stream_entry_id_start =
        static_cast<RespBulkString &>(*args.at(arg_idx + i + pairs_count))
            .getValue();
    store_keys.push_back(store_key);
    entry_ids_start.push_back(stream_entry_id_start);
  }
  auto [timed_out, stream_entries_map] =
      RedisStore::instance().getStreamEntriesAfterAny(
          store_keys, entry_ids_start, timeout_ms);

  if (timed_out) {
    result.push_back(std::make_unique<RespBulkString>());
    return result;
  }

  auto resp_array = std::make_unique<RespArray>();

  for (const auto &store_key : store_keys) {
    if (stream_entries_map.contains(store_key)) {
      auto stream_entries = stream_entries_map[store_key];
      auto key_array = std::make_unique<RespArray>();
      key_array->add(std::make_unique<RespBulkString>(store_key))
          ->add(serializeStreamEntries(stream_entries));
      resp_array->add(std::move(key_array));
    }
  }

  result.push_back(std::move(resp_array));
  return result;
}
