#ifndef WEBSERV_HPP_
#define WEBSERV_HPP_

#include <memory>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <deque>
#include "Socket.hpp"
#include "Connection.hpp"
#include <signal.h>

#define DEFAULT_CONF "conf/default.conf"
#define TIMEOUT 500

class WebServ {
 public:
  WebServ(const char* config_file);
  WebServ(const WebServ& other)            = delete;
  WebServ& operator=(const WebServ& other) = delete;

  ~WebServ() = default;

  int   Init();
  void  Run();
  void  AddNewConnection(pollfd& fd, std::unique_ptr<Connection> connection);
  void  SwitchCgiToReceive(int olg_cgi_fd, int new_cgi_fd);
  void  SwitchClientToSend(int fd);

 private:
  void        PollAvailableFDs();
  void        CheckForNewConnection(int fd, short revents, int i);
  void        ReceiveData(Connection& connection, const int& i);
  void        SendData(Connection& connection, const int& i);
  void        CloseConnection(int fd, const int& i);
  void        CloseAllConnections();
  std::string ToString() const;

  const std::string                          conf_;
  std::deque<Socket>                         sockets_;
  std::vector<pollfd>                        pollFDs_;
  std::map<int, std::unique_ptr<Connection>> connections_;
};
#endif
