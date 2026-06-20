#include "commands/LrangeCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/ListValue.hpp"
#include <string>
#include <utility>
#include <variant>
#include <vector>

std::vector<RespValue>
LrangeCommand::doExecute(const std::vector<RespValue> &args,
                         RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  const int start_idx = std::stoi(getStringValue(args.at(1)));
  const int end_idx = std::stoi(getStringValue(args.at(2)));
  auto val = store.get(store_key);
  RespArray resp_array;
  if (val) {
    auto *list_val = std::get_if<ListValue>(&val.value());
    if (list_val != nullptr) {
      auto list_elements = list_val->getElementsInRange(start_idx, end_idx);
      for (const auto &list_el : list_elements) {
        resp_array.add(RespBulkString(list_el));
      }
    }
  }
  result.emplace_back(std::move(resp_array));
  return result;
}
