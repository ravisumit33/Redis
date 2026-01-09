#pragma once

#include "Command.hpp"

class AppContext;

class LpushCommand : public Command {
public:
  LpushCommand() : Command(Type::LPUSH) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          AppContext &context);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() >= 2;
  }
};
