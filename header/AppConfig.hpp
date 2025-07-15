#pragma once

#include <string>

struct AppConfig {
  unsigned port;
  std::string replicaOf;
  std::string master_replid;
  int master_repl_offset;
};
