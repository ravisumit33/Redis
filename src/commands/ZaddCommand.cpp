#include "commands/ZaddCommand.hpp"
#include "RespType.hpp"
#include "redis_store/RedisStore.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

std::vector<RespValue>
ZaddCommand::doExecute(const std::vector<RespValue> &args, RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto score = std::stod(getStringValue(args.at(1)));
  auto member = getStringValue(args.at(2));

  auto added_count = store.addMemberToSet(store_key, score, member);
  std::cout << "Member: " << member << " with score: " << score
            << " added to set with key: " << store_key << '\n';
  result.emplace_back(RespInt(static_cast<int64_t>(added_count)));
  return result;
}
