#include "commands/ZaddCommand.hpp"
#include "AppContext.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include <iostream>

std::vector<RespValue>
ZaddCommand::doExecute(const std::vector<RespValue> &args,
                       AppContext &context) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto score = std::stod(getStringValue(args.at(1)));
  auto member = getStringValue(args.at(2));

  auto added_count =
      context.getRedisStore().addMemberToSet(store_key, score, member);
  std::cout << "Member: " << member << " with score: " << score
            << " added to set with key: " << store_key << '\n';
  result.emplace_back(RespInt(static_cast<int64_t>(added_count)));
  return result;
}

std::vector<RespValue>
ZaddCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext());
}

std::vector<RespValue>
ZaddCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext());
}
