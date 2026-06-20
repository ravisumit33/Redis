#include "commands/ZremCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

std::vector<RespValue>
ZremCommand::doExecute(const std::vector<RespValue> &args, RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto member = getStringValue(args.at(1));

  std::size_t removed_cnt = 0;
  try {
    removed_cnt = store.removeMemberFromSet(store_key, member);
  } catch (...) {
  }

  result.emplace_back(RespInt(static_cast<int64_t>(removed_cnt)));
  return result;
}
