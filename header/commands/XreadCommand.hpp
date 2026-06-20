#pragma once

#include "Command.hpp"

class RedisStore;

class XreadCommand : public Command {
public:
  XreadCommand() : Command(Type::XREAD) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          RedisStore &store);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    std::size_t nargs = args.size();
    return (nargs >= 3) && (nargs % 2 == 1);
  }
};
