#pragma once

#include "Command.hpp"
#include "Registrar.hpp"

class LpushCommand : public Command {
public:
  LpushCommand() : Command(LPUSH) {}

private:
  static CommandRegistrar<LpushCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() >= 2;
  }
};