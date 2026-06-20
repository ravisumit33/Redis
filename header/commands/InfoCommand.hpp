#pragma once

#include "Command.hpp"

class AppConfig;
class ReplicationManager;

class InfoCommand : public Command {
public:
  InfoCommand() : Command(Type::INFO) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          const AppConfig &config,
                                          ReplicationManager &repl);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 1;
  }
};
