#pragma once

#include "Command.hpp"

class PingCommand : public Command {
public:
  PingCommand() : Command(PING) {}

private:
  static CommandRegistrar<PingCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 0;
  }
};
