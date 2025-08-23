#pragma once

#include "Command.hpp"

class ZscoreCommand : public Command {
public:
  ZscoreCommand() : Command(ZSCORE) {}

private:
  static CommandRegistrar<ZscoreCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2;
  }
};
