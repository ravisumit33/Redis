#pragma once

#include "Command.hpp"

class UnsubscribeCommand : public Command {
public:
  UnsubscribeCommand() : Command(Type::UNSUBSCRIBE) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

private:
  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 1;
  }
};
