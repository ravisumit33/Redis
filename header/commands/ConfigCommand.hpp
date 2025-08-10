#pragma once

#include "Command.hpp"
#include "Registrar.hpp"

class ConfigCommand : public Command {
public:
  ConfigCommand() : Command(CONFIG) {}

private:
  static CommandRegistrar<ConfigCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2;
  }
};