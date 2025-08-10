#pragma once

#include "Command.hpp"
#include "Registrar.hpp"

class DiscardCommand : public Command {
public:
  DiscardCommand() : Command(DISCARD) {}

private:
  static CommandRegistrar<DiscardCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 0;
  }
};