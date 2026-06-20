#include "commands/TypeCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/RedisValue.hpp"
#include <vector>

std::vector<RespValue>
TypeCommand::doExecute(const std::vector<RespValue> &args, RedisStore &store) {
  std::vector<RespValue> result;
  auto arg1 = getStringValue(args.at(0));
  auto store_val_ptr = store.get(arg1);
  if (!store_val_ptr) {
    result.emplace_back(RespString("none"));
    return result;
  }
  auto val_type = getRedisValueTypeStr(store_val_ptr.value());
  result.emplace_back(RespString(val_type));
  return result;
}
