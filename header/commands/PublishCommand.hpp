#pragma once

#include "Command.hpp"

class PublishCommand : public Command {
public:
  PublishCommand() : Command(PUBLISH) {}

private:
  static CommandRegistrar<PublishCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2;
  }
};
