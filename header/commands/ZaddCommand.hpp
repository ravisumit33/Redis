#pragma once

#include "Command.hpp"

class ZaddCommand : public Command {
public:
  ZaddCommand() : Command(ZADD) {}

private:
  static CommandRegistrar<ZaddCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 3;
  }
};
