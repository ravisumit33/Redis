#include "commands/ZscoreCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/SetValue.hpp"
#include <memory>

CommandRegistrar<ZscoreCommand> ZscoreCommand::registrar("ZSCORE");

std::vector<std::unique_ptr<RespType>>
ZscoreCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                           Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto member = static_cast<RespBulkString &>(*args.at(1)).getValue();

  auto val = RedisStore::instance().get(store_key);
  std::string score;
  if (val) {
    auto set_val = static_cast<SetValue &>(*(val.value()));
    try {
      score = std::to_string(set_val.getScore(member));
    } catch (...) {
    }
  }

  if (!score.empty()) {
    result.push_back(std::make_unique<RespBulkString>(score));
  } else {
    result.push_back(std::make_unique<RespBulkString>());
  }
  return result;
}
