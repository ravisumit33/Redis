#include "commands/ZscoreCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "redis_store/values/SetValue.hpp"
#include <cstdint>
#include <iomanip>
#include <sstream>

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

std::vector<RespValue>
ZscoreCommand::executeOnImpl(const std::vector<RespValue> &args,
                             ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
ZscoreCommand::executeOnImpl(const std::vector<RespValue> &args,
                             ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
