#include "AppConfig.hpp"
#include "ArgParser.hpp"
#include "RedisServer.hpp"
#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <string>

std::string generateRandomHexId() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> dis(0, 255);

  std::ostringstream oss;
  for (size_t i = 0; i < 20; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(dis(gen));
  }
  return oss.str();
}

int main(int argc, char **argv) {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  ArgParser parser("Redis Server");
  parser.addOption("port", "Server port", "6379");
  parser.addOption("replicaof", "Master server descripton if replica");

  try {
    parser.parse(argc, argv);
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n" << std::endl;
    parser.printHelp(argv[0]);
    return 1;
  }

  std::optional<MasterMetadata> mmetadata = std::nullopt;
  std::optional<SlaveMetadata> smetadata = std::nullopt;
  auto replicaof = parser.get<std::string>("replicaof");
  if (replicaof.empty()) {
    mmetadata = {.master_replid = generateRandomHexId(),
                 .master_repl_offset = 0};
  } else {
    std::string master_host;
    unsigned master_port;
    std::istringstream ss(replicaof);
    ss >> master_host >> master_port;
    smetadata = {.master_host = master_host, .master_port = master_port};
  }

  AppConfig config(
      static_cast<unsigned>(std::stoul(parser.get<std::string>("port"))),
      mmetadata, smetadata);

  try {
    RedisServer server(config);
    server.start();
  } catch (const std::exception &e) {
    std::cerr << "Server error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
