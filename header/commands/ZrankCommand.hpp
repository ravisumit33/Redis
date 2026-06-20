#pragma once

#include "Command.hpp"

class RedisStore;

class ZrankCommand : public Command {
public:
  ZrankCommand() : Command(Type::ZRANK) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          RedisStore &store);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 2;
  }
};
