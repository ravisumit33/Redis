#pragma once

#include "Command.hpp"

class ReplicationManager;

class PsyncCommand : public Command {
public:
  PsyncCommand() : Command(Type::PSYNC) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          ReplicationManager &repl);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 2;
  }
};
