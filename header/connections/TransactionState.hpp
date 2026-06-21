#pragma once

#include "Command.hpp"
#include "CommandExecutor.hpp"
#include "RespType.hpp"
#include <utility>
#include <vector>

class TransactionState {
public:
  void begin() {
    m_in_transaction = true;
    m_queued_commands.clear();
  }

  bool inTransaction() const { return m_in_transaction; }

  void queue(Command command, std::vector<RespValue> args) {
    m_queued_commands.emplace_back(command, std::move(args));
  }

  void reset() {
    m_queued_commands.clear();
    m_in_transaction = false;
  }

  template <typename Ctx> std::vector<RespValue> executeQueued(Ctx &ctx) {
    RespArray resp_array;
    for (auto &[queued_cmd, queued_args] : m_queued_commands) {
      auto sub_result = executeCommand(queued_cmd, queued_args, ctx);
      for (auto &res : sub_result) {
        resp_array.add(std::move(res));
      }
    }
    reset();
    std::vector<RespValue> result;
    result.emplace_back(std::move(resp_array));
    return result;
  }

private:
  bool m_in_transaction = false;
  std::vector<std::pair<Command, std::vector<RespValue>>> m_queued_commands;
};
