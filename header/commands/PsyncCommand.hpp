#pragma once

#include "Command.hpp"
#include "Registrar.hpp"

class PsyncCommand : public Command {
public:
  PsyncCommand() : Command(PSYNC) {}

private:
  static CommandRegistrar<PsyncCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2;
  }
};