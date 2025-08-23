#pragma once

#include "Command.hpp"

class ZrangeCommand : public Command {
public:
  ZrangeCommand() : Command(ZRANGE) {}

private:
  static CommandRegistrar<ZrangeCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 3;
  }
};
