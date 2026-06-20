#pragma once

#include "Command.hpp"

class RedisChannelManager;

class PublishCommand : public Command {
public:
  PublishCommand() : Command(Type::PUBLISH) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          RedisChannelManager &channels);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 2;
  }
};
