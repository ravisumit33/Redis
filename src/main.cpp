#include "RedisServer.hpp"
#include <exception>
#include <iostream>

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  try {
    unsigned port = 6379;
    if (argc > 1) {
      port = std::stoi(argv[2]);
    }
    RedisServer server(port);
    server.start();
  } catch (const std::exception &e) {
    std::cerr << "Server error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
