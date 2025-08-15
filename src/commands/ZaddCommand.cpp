#include "commands/ZaddCommand.hpp"
#include "Connection.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"
#include <iostream>
#include <memory>

CommandRegistrar<ZaddCommand> ZaddCommand::registrar("ZADD");

std::vector<std::unique_ptr<RespType>>
ZaddCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto score = std::stod(static_cast<RespBulkString &>(*args.at(1)).getValue());
  auto member = static_cast<RespBulkString &>(*args.at(2)).getValue();

  auto added_count =
      RedisStore::instance().addMemberToSet(store_key, score, member);
  std::cout << "Member: " << member << "with score: " << score
            << " added to set with key: " << store_key << std::endl;
  result.push_back(std::make_unique<RespInt>(added_count));
  return result;
}
