#include "AppConfig.hpp"
#include "utils/ArgParser.hpp"
#include "RedisServer.hpp"
#include "ReplicationState.hpp"
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

int main(int argc, char **argv) {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  ArgParser parser("Redis Server");
  parser.addOption("port", "Server port", "6379");
  parser.addOption("replicaof", "Master server descripton if replica");
  parser.addOption("dir", "Path to directory contain redis files");
  parser.addOption("dbfilename", "RDB file name");

  try {
    parser.parse(argc, argv);
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n" << std::endl;
    parser.printHelp(argv[0]);
    return 1;
  }

  std::optional<SlaveConfig> slave_config = std::nullopt;
  std::optional<MasterConfig> master_config = std::nullopt;
  auto replicaof = parser.get<std::string>("replicaof");
  if (replicaof.empty()) {
    ReplicationManager::getInstance().initAsMaster();
    auto redis_dir = parser.get<std::string>("dir");
    auto db_filename = parser.get<std::string>("dbfilename");
    master_config = {.dir = redis_dir, .dbfilename = db_filename};
  } else {
    std::string master_host;
    unsigned master_port;
    std::istringstream ss(replicaof);
    ss >> master_host >> master_port;
    slave_config = {.master_host = master_host, .master_port = master_port};
    ReplicationManager::getInstance().initAsSlave();
  }

  AppConfig config(
      static_cast<unsigned>(std::stoul(parser.get<std::string>("port"))),
      master_config, slave_config);

  try {
    auto server = std::make_shared<RedisServer>(config);
    server->start();
  } catch (const std::exception &e) {
    std::cerr << "FATAL: Server startup failed: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Redis server shutting down..." << std::endl;

  return 0;
}
