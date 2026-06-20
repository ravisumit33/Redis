#include "commands/ZscoreCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/SetValue.hpp"
#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

std::vector<RespValue>
ZscoreCommand::doExecute(const std::vector<RespValue> &args,
                         RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto member = getStringValue(args.at(1));
  constexpr uint8_t required_precision = 15;
  auto val = store.get(store_key);
  std::string score_str;
  if (val) {
    auto *set_val = std::get_if<SetValue>(&val.value());
    if (set_val != nullptr) {
      try {
        auto score = set_val->getScore(member);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(required_precision) << score;
        score_str = oss.str();
      } catch (...) {
      }
    }
  }

  if (!score_str.empty()) {
    result.emplace_back(RespBulkString(score_str));
  } else {
    result.emplace_back(RespBulkString());
  }
  return result;
}
