#pragma once

#include "Command.hpp"
#include "Registrar.hpp"

class LpopCommand : public Command {
public:
  LpopCommand() : Command(LPOP) {}

private:
  static CommandRegistrar<LpopCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1 || args.size() == 2;
  }
};