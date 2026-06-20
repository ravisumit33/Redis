#pragma once

#include "Command.hpp"

class GeoaddCommand : public Command {
public:
  GeoaddCommand() : Command(Type::GEOADD) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 4;
  }
};
