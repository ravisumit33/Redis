#include "commands/TypeCommand.hpp"
#include "Connection.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"

CommandRegistrar<TypeCommand> TypeCommand::registrar("TYPE");

std::vector<std::unique_ptr<RespType>>
TypeCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto arg1 = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto store_val_ptr = RedisStore::instance().get(arg1);
  if (!store_val_ptr) {
    result.push_back(std::make_unique<RespString>("none"));
    return result;
  }
  auto val_type = store_val_ptr.value()->getTypeStr();
  result.push_back(std::make_unique<RespString>(val_type));
  return result;
}
