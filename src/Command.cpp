#include "Command.hpp"
#include <unordered_set>

bool Command::isWriteCommand() const {
  static const std::unordered_set<Type> write_commands = {SET};
  return write_commands.contains(getType());
}

bool Command::isControlCommand() const {
  static const std::unordered_set<Type> control_commands = {EXEC, DISCARD};
  return control_commands.contains(getType());
}