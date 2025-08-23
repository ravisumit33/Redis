#include "commands/ZcardCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include "redis_store/values/SetValue.hpp"
#include <memory>

CommandRegistrar<ZcardCommand> ZcardCommand::registrar("ZCARD");

std::vector<std::unique_ptr<RespType>>
ZcardCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();

  std::size_t set_size = 0;
  auto val = RedisStore::instance().get(store_key);
  if (val) {
    auto set_val = static_cast<SetValue &>(*(val.value()));
    set_size = set_val.size();
  }
  result.push_back(std::make_unique<RespInt>(set_size));
  return result;
}
