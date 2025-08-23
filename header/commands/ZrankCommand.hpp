#pragma once

#include "Command.hpp"

class ZrankCommand : public Command {
public:
  ZrankCommand() : Command(ZRANK) {}

private:
  static CommandRegistrar<ZrankCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2;
  }
};
