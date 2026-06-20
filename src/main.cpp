#include "AppConfig.hpp"
#include "RedisServer.hpp"
#include "utils/ArgParser.hpp"
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>

int main(int argc, char **argv) {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  auto args = std::span(argv, static_cast<size_t>(argc));
  const char *program_name = args[0];

  ArgParser parser("Redis Server");
  parser.addOption("port", "Server port", "6379");
  parser.addOption("replicaof", "Master server descripton if replica");
  parser.addOption("dir", "Path to directory contain redis files");
  parser.addOption("dbfilename", "RDB file name");

  try {
    parser.parse(args);
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n" << '\n';
    parser.printHelp(program_name);
    return 1;
  }

  std::optional<SlaveConfig> slave_config = std::nullopt;
  std::optional<MasterConfig> master_config = std::nullopt;
  auto replicaof = parser.get<std::string>("replicaof");
  if (replicaof.empty()) {
    auto redis_dir = parser.get<std::string>("dir");
    auto db_filename = parser.get<std::string>("dbfilename");
    master_config = {.dir = redis_dir, .dbfilename = db_filename};
  } else {
    std::string master_host;
    unsigned master_port{};
    std::istringstream s_stream(replicaof);
    s_stream >> master_host >> master_port;
    slave_config = {.master_host = master_host, .master_port = master_port};
  }

  AppConfig config(
      static_cast<unsigned>(std::stoul(parser.get<std::string>("port"))),
      master_config, slave_config);

  try {
    auto server = std::make_shared<RedisServer>(config);
    server->start();
  } catch (const std::exception &e) {
    std::cerr << "FATAL: Server startup failed: " << e.what() << '\n';
    return 1;
  }

  std::cout << "Redis server shutting down..." << '\n';

  return 0;
}
