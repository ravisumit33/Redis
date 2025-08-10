#pragma once

#include "Command.hpp"

class XaddCommand : public Command {
public:
  XaddCommand() : Command(XADD) {}

private:
  static CommandRegistrar<XaddCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    std::size_t nargs = args.size();
    return (nargs >= 4 && nargs % 2 == 0);
  }
};
