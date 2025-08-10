#pragma once

#include "Command.hpp"

class BlpopCommand : public Command {
public:
  BlpopCommand() : Command(BLPOP) {}

private:
  static CommandRegistrar<BlpopCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2;
  }
};
