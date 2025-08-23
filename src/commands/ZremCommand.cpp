#include "commands/ZremCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include <memory>

CommandRegistrar<ZremCommand> ZremCommand::registrar("ZREM");

std::vector<std::unique_ptr<RespType>>
ZremCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto member = static_cast<RespBulkString &>(*args.at(1)).getValue();

  std::size_t removed_cnt = 0;
  try {
    removed_cnt = RedisStore::instance().removeMemberFromSet(store_key, member);
  } catch (...) {
  }

  result.push_back(std::make_unique<RespInt>(removed_cnt));
  return result;
}
