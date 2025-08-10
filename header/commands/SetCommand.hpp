#pragma once

#include "Command.hpp"

class SetCommand : public Command {
public:
  SetCommand() : Command(SET) {}

private:
  static CommandRegistrar<SetCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2 || args.size() == 4;
  }
};
