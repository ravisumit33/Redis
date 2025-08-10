#pragma once

#include "Command.hpp"
#include "Registrar.hpp"

class ReplConfCommand : public Command {
public:
  ReplConfCommand() : Command(REPLCONF) {}

private:
  static CommandRegistrar<ReplConfCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() >= 2;
  }
};