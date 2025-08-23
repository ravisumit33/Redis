#include "commands/LlenCommand.hpp"
#include "Connection.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/ListValue.hpp"

CommandRegistrar<LlenCommand> LlenCommand::registrar("LLEN");

std::vector<std::unique_ptr<RespType>>
LlenCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto val = RedisStore::instance().get(store_key);
  if (!val) {
    result.push_back(std::make_unique<RespInt>(0));
    return result;
  }
  auto list_val = static_cast<ListValue &>(*(val.value()));
  result.push_back(std::make_unique<RespInt>(list_val.size()));
  return result;
}
