#include "commands/DiscardCommand.hpp"
#include "RespType.hpp"
#include "connections/TransactionState.hpp"
#include <vector>

std::vector<RespValue>
DiscardCommand::doExecute(const std::vector<RespValue> & /*args*/,
                          TransactionState &txn) {
  std::vector<RespValue> result;
  if (!txn.inTransaction()) {
    result.emplace_back(RespError("DISCARD without MULTI"));
    return result;
  }
  txn.reset();
  result.emplace_back(RespString("OK"));
  return result;
}
