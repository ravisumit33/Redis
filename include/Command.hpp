#pragma once

#include "Registry.hpp"
#include "RespType.hpp"
#include "commands/BlpopCommand.hpp"
#include "commands/ConfigCommand.hpp"
#include "commands/DiscardCommand.hpp"
#include "commands/EchoCommand.hpp"
#include "commands/ExecCommand.hpp"
#include "commands/GeoaddCommand.hpp"
#include "commands/GetCommand.hpp"
#include "commands/IncrCommand.hpp"
#include "commands/InfoCommand.hpp"
#include "commands/KeysCommand.hpp"
#include "commands/LlenCommand.hpp"
#include "commands/LpopCommand.hpp"
#include "commands/LpushCommand.hpp"
#include "commands/LrangeCommand.hpp"
#include "commands/MultiCommand.hpp"
#include "commands/PingCommand.hpp"
#include "commands/PsyncCommand.hpp"
#include "commands/PublishCommand.hpp"
#include "commands/ReplConfCommand.hpp"
#include "commands/RpushCommand.hpp"
#include "commands/SetCommand.hpp"
#include "commands/SubscribeCommand.hpp"
#include "commands/TypeCommand.hpp"
#include "commands/UnsubscribeCommand.hpp"
#include "commands/WaitCommand.hpp"
#include "commands/XaddCommand.hpp"
#include "commands/XrangeCommand.hpp"
#include "commands/XreadCommand.hpp"
#include "commands/ZaddCommand.hpp"
#include "commands/ZcardCommand.hpp"
#include "commands/ZrangeCommand.hpp"
#include "commands/ZrankCommand.hpp"
#include "commands/ZremCommand.hpp"
#include "commands/ZscoreCommand.hpp"
#include <istream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

class AppContext;

using Command = std::variant<
    BlpopCommand, ConfigCommand, DiscardCommand, EchoCommand, ExecCommand,
    GeoaddCommand, GetCommand, IncrCommand, InfoCommand, KeysCommand,
    LlenCommand, LpopCommand, LpushCommand, LrangeCommand, MultiCommand,
    PingCommand, PsyncCommand, PublishCommand, ReplConfCommand, RpushCommand,
    SetCommand, SubscribeCommand, TypeCommand, UnsubscribeCommand, WaitCommand,
    XaddCommand, XrangeCommand, XreadCommand, ZaddCommand, ZcardCommand,
    ZrangeCommand, ZrankCommand, ZremCommand, ZscoreCommand>;

bool isWriteCommand(const Command &cmd);
bool isControlCommand(const Command &cmd);
bool isSubscribedModeCommand(const Command &cmd);
std::string_view getCommandName(const Command &cmd);

using CommandRegistry = Registry<std::string, Command>;
void registerCommands(CommandRegistry &registry);

std::pair<Command, std::vector<RespValue>> parseCommand(std::istream &in_stream,
                                                        AppContext &context);
