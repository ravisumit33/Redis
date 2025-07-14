#include "AppConfig.hpp"
#include "ArgParser.hpp"
#include "RedisServer.hpp"
#include <exception>
#include <iostream>
#include <string>

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

  std::cout << "Port: " << parser.get<std::string>("port") << std::endl;
  std::cout << "replicaOf: " << parser.get<std::string>("replicaof")
            << std::endl;

  AppConfig config{.port = static_cast<unsigned>(
                       std::stoul(parser.get<std::string>("port"))),
                   .replicaOf = parser.get<std::string>("replicaof")};

  try {
    unsigned port = 6379;
    if (argc == 3 && std::string(argv[1]) == "--port") {
      port = std::stoi(argv[2]);
    }
    RedisServer server(config);
    server.start();
  } catch (const std::exception &e) {
    std::cerr << "Server error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
