#pragma once

#include "Command.hpp"
#include "Registrar.hpp"

class TypeCommand : public Command {
public:
  TypeCommand() : Command(TYPE) {}

private:
  static CommandRegistrar<TypeCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1;
  }
};