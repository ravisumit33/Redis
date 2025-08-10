#pragma once

#include "Command.hpp"

class RpushCommand : public Command {
public:
  RpushCommand() : Command(RPUSH) {}

private:
  static CommandRegistrar<RpushCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() >= 2;
  }
};
