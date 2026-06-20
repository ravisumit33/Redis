#pragma once

#include "Command.hpp"

class RedisStore;

class XrangeCommand : public Command {
public:
  XrangeCommand() : Command(Type::XRANGE) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          RedisStore &store);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 3;
  }
};
