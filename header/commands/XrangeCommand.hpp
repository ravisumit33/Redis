#pragma once

#include "Command.hpp"

class XrangeCommand : public Command {
public:
  XrangeCommand() : Command(XRANGE) {}

private:
  static CommandRegistrar<XrangeCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 3;
  }
};
