#pragma once

#include "Command.hpp"

class MultiCommand : public Command {
public:
  MultiCommand() : Command(MULTI) {}

private:
  static CommandRegistrar<MultiCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 0;
  }
};
