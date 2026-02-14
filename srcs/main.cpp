#include <iostream>
#include "Logger.hpp"
#include "WebServ.hpp"

int main(int ac, char** av) {
  if (ac > 2) {
    logError("Usage: /webserv [configuration file]");
    return 1;
  }

  WebServ webServ(av[1]);
  if (webServ.Init()) {
    logError("Failed to initialize the server. Terminating.");
    return 1;
  }
  webServ.Run();
  return 0;
}
