#pragma once

#include "Command.hpp"

class SubscribeCommand : public Command {
public:
  SubscribeCommand() : Command(Type::SUBSCRIBE) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

private:
  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 1;
  }
};
