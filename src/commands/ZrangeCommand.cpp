#include "commands/ZrangeCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/SetValue.hpp"

CommandRegistrar<ZrangeCommand> ZrangeCommand::registrar("ZRANGE");

std::vector<std::unique_ptr<RespType>>
ZrangeCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                           Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  int start_idx =
      std::stoi(static_cast<RespBulkString &>(*args.at(1)).getValue());
  int end_idx =
      std::stoi(static_cast<RespBulkString &>(*args.at(2)).getValue());
  auto val = RedisStore::instance().get(store_key);
  auto resp_array = std::make_unique<RespArray>();
  if (val) {
    auto set_val = static_cast<SetValue &>(*(val.value()));
    auto set_members = set_val.getElementsInRange(start_idx, end_idx);
    for (const auto &set_member : set_members) {
      resp_array->add(std::make_unique<RespBulkString>(set_member));
    }
  }
  result.push_back(std::move(resp_array));
  return result;
}
