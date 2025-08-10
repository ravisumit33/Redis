#pragma once

#include "Command.hpp"
#include "Registrar.hpp"

class GetCommand : public Command {
public:
  GetCommand() : Command(GET) {}

private:
  static CommandRegistrar<GetCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1;
  }
};