#include "commands/BlpopCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include <string>
#include <utility>
#include <vector>

std::vector<RespValue>
BlpopCommand::doExecute(const std::vector<RespValue> &args, RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto timeout_s = std::stod(getStringValue(args.at(1)));
  auto [timed_out, popped_elements] =
      store.removeListElementsAtBegin(store_key, 1, timeout_s);
  if (timed_out) {
    result.emplace_back(RespArray::null());
    return result;
  }
  RespArray resp_array;
  resp_array.add(RespBulkString(store_key))
      .add(RespBulkString(popped_elements.at(0)));

  result.emplace_back(std::move(resp_array));
  return result;
}
