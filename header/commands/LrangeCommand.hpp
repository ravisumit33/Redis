#pragma once

#include "Command.hpp"
#include "Registrar.hpp"

class LrangeCommand : public Command {
public:
  LrangeCommand() : Command(LRANGE) {}

private:
  static CommandRegistrar<LrangeCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 3;
  }
};