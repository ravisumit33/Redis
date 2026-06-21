#include "commands/MultiCommand.hpp"
#include "RespType.hpp"
#include "connections/TransactionState.hpp"
#include <vector>

std::vector<RespValue>
MultiCommand::doExecute(const std::vector<RespValue> & /*args*/,
                        TransactionState &txn) {
  std::vector<RespValue> result;
  txn.begin();
  result.emplace_back(RespString("OK"));
  return result;
}
