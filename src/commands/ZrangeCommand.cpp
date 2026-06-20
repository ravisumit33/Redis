#include "commands/ZrangeCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/SetValue.hpp"
#include <string>
#include <utility>
#include <variant>
#include <vector>

std::vector<RespValue>
ZrangeCommand::doExecute(const std::vector<RespValue> &args,
                         RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  const int start_idx = std::stoi(getStringValue(args.at(1)));
  const int end_idx = std::stoi(getStringValue(args.at(2)));
  auto val = store.get(store_key);
  RespArray resp_array;
  if (val) {
    auto *set_val = std::get_if<SetValue>(&val.value());
    if (set_val != nullptr) {
      auto set_members = set_val->getElementsInRange(start_idx, end_idx);
      for (const auto &set_member : set_members) {
        resp_array.add(RespBulkString(set_member));
      }
    }
  }
  result.emplace_back(std::move(resp_array));
  return result;
}
