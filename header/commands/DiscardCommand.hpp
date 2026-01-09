#pragma once

#include "Command.hpp"

class DiscardCommand : public Command {
public:
  DiscardCommand() : Command(Type::DISCARD) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

private:
  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 0;
  }
};
