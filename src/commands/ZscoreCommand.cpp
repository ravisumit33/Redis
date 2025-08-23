#include "commands/ZscoreCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/SetValue.hpp"
#include <iomanip>
#include <memory>
#include <sstream>

CommandRegistrar<ZscoreCommand> ZscoreCommand::registrar("ZSCORE");

std::vector<std::unique_ptr<RespType>>
ZscoreCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                           Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto member = static_cast<RespBulkString &>(*args.at(1)).getValue();

  auto val = RedisStore::instance().get(store_key);
  std::string score_str;
  if (val) {
    auto set_val = static_cast<SetValue &>(*(val.value()));
    try {
      auto score = set_val.getScore(member);
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(15) << score;
      score_str = oss.str();
    } catch (...) {
    }
  }

  if (!score_str.empty()) {
    result.push_back(std::make_unique<RespBulkString>(score_str));
  } else {
    result.push_back(std::make_unique<RespBulkString>());
  }
  return result;
}
