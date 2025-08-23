#pragma once

#include "Command.hpp"

class ZremCommand : public Command {
public:
  ZremCommand() : Command(ZSCORE) {}

private:
  static CommandRegistrar<ZremCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2;
  }
};
