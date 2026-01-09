#pragma once

#include "Command.hpp"

class AppContext;

class XaddCommand : public Command {
public:
  XaddCommand() : Command(Type::XADD) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          AppContext &context);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    std::size_t nargs = args.size();
    return (nargs >= 4 && nargs % 2 == 0);
  }
};
