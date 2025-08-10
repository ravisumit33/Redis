#include "commands/LrangeCommand.hpp"
#include "Connection.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"

CommandRegistrar<LrangeCommand> LrangeCommand::registrar("LRANGE");

std::vector<std::unique_ptr<RespType>>
LrangeCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
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
    auto list_val = static_cast<ListValue &>(*(val.value()));
    auto list_elements = list_val.getElementsInRange(start_idx, end_idx);
    for (const auto &list_el : list_elements) {
      resp_array->add(std::make_unique<RespBulkString>(list_el));
    }
  }
  result.push_back(std::move(resp_array));
  return result;
}
