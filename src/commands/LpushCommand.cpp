#include "commands/LpushCommand.hpp"
#include "AppContext.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include <algorithm>
#include <vector>

std::vector<RespValue>
LpushCommand::doExecute(const std::vector<RespValue> &args,
                        AppContext &context) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  std::vector<std::string> elements;
  std::ranges::transform(args.begin() + 1, args.end(),
                         std::back_inserter(elements),
                         [](const auto &arg) { return getStringValue(arg); });
  std::ranges::reverse(elements);
  auto list_size =
      context.getRedisStore().addListElementsAtBegin(store_key, elements);
  result.emplace_back(RespInt(static_cast<int64_t>(list_size)));
  return result;
}

std::vector<RespValue>
LpushCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ClientConnection &connection) {
  return doExecute(args, connection.getContext());
}

std::vector<RespValue>
LpushCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ServerConnection &connection) {
  return doExecute(args, connection.getContext());
}
