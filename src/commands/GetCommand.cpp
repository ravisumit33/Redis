#include "commands/GetCommand.hpp"
#include "Connection.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"

CommandRegistrar<GetCommand> GetCommand::registrar("GET");

std::vector<std::unique_ptr<RespType>>
GetCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                        Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;

  auto key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto stored_value = RedisStore::instance().get(key);
  if (!stored_value) {
    result.push_back(std::make_unique<RespBulkString>());
    return result;
  }

  auto return_value = std::move(stored_value.value());
  if (return_value->getType() != RedisStoreValue::STRING) {
    result.push_back(std::make_unique<RespError>("Invalid Key"));
    return result;
  }
  result.push_back(std::make_unique<RespBulkString>(
      static_cast<StringValue &>(*return_value).getValue()));
  return result;
}
