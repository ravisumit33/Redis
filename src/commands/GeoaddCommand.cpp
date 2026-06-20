#include "commands/GeoaddCommand.hpp"
#include "RespType.hpp"
#include <vector>

std::vector<RespValue>
GeoaddCommand::doExecute(const std::vector<RespValue> & /*args*/) {
  std::vector<RespValue> result;
  result.emplace_back(RespInt(1));
  return result;
}
