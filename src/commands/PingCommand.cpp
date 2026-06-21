#include "commands/PingCommand.hpp"
#include "RespType.hpp"
#include <utility>
#include <vector>

std::vector<RespValue> PingCommand::doExecute(bool subscribed_mode) {
  std::vector<RespValue> result;
  if (subscribed_mode) {
    RespArray resp_array;
    resp_array.add(RespBulkString("pong")).add(RespBulkString(""));
    result.emplace_back(std::move(resp_array));
    return result;
  }
  result.emplace_back(RespString("PONG"));
  return result;
}
