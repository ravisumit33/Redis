#include "commands/LpushCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

std::vector<RespValue>
LpushCommand::doExecute(const std::vector<RespValue> &args, RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  std::vector<std::string> elements;
  std::ranges::transform(args.begin() + 1, args.end(),
                         std::back_inserter(elements),
                         [](const auto &arg) { return getStringValue(arg); });
  std::ranges::reverse(elements);
  auto list_size = store.addListElementsAtBegin(store_key, elements);
  result.emplace_back(RespInt(static_cast<int64_t>(list_size)));
  return result;
}
