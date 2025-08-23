#include "commands/KeysCommand.hpp"
#include "Connection.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"

CommandRegistrar<KeysCommand> KeysCommand::registrar("KEYS");

std::vector<std::unique_ptr<RespType>>
KeysCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto arg1 = static_cast<RespBulkString &>(*args.at(0)).getValue();
  if (arg1 != "*") {
    result.push_back(std::make_unique<RespError>("Invalid argument: " + arg1));
    return result;
  }
  auto store_keys = RedisStore::instance().getKeys();
  auto resp_array = std::make_unique<RespArray>();
  for (const auto &store_key : store_keys) {
    resp_array->add(std::make_unique<RespBulkString>(store_key));
  }
  result.push_back(std::move(resp_array));
  return result;
}
