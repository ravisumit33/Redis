#pragma once

#include "Command.hpp"

class XreadCommand : public Command {
public:
  XreadCommand() : Command(XREAD) {}

private:
  static CommandRegistrar<XreadCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    std::size_t nargs = args.size();
    return (nargs >= 3) && (nargs % 2 == 1);
  }
};
