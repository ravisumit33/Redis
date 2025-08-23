#include "commands/IncrCommand.hpp"
#include "Connection.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/StringValue.hpp"
#include <string>

CommandRegistrar<IncrCommand> IncrCommand::registrar("INCR");

std::vector<std::unique_ptr<RespType>>
IncrCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();

  auto store_val_ptr = RedisStore::instance().get(store_key);
  if (!store_val_ptr) {
    RedisStore::instance().setString(store_key, "1");
    result.push_back(std::make_unique<RespInt>(1));
    return result;
  }

  auto store_val =
      static_cast<StringValue &>(*(store_val_ptr.value())).getValue();
  int64_t int_val;
  try {
    int_val = std::stoll(store_val);
  } catch (...) {
    result.push_back(
        std::make_unique<RespError>("value is not an integer or out of range"));
    return result;
  }

  ++int_val;
  RedisStore::instance().setString(store_key, std::to_string(int_val));
  result.push_back(std::make_unique<RespInt>(int_val));
  return result;
}
