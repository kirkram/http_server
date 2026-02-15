#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

#include "Connection.hpp"
#include "HttpParser.hpp"
#include "HttpResponse.hpp"
#include <fstream>
#include <sys/socket.h>

class Socket;
class VirtualHost;
class WebServ;
class CgiConnection;

class ClientConnection : public Connection {
 public:
  ClientConnection(int fd, Socket& sock, WebServ& webserv);
  ClientConnection(const ClientConnection& other)             = delete;
  ~ClientConnection() override;

  int                       ReceiveData(pollfd& poll) override;
  int                       SendData(pollfd& poll) override;
  void                      ResetClientConnection();
  std::vector<std::string>  PrepareCgiEvniron();

 private:
  friend HttpParser;
  friend HttpResponse;
  friend CgiConnection;

  enum class Stage { kHeader,
                     kBody,
                     kCgi,
                     kResponse,
                     kSending,
                     kDrain };

  Stage         stage_ = Stage::kHeader;
  std::string   status_ = "200";
  Socket&       sock_;
  WebServ&      webserv_;
  VirtualHost*  vhost_ = nullptr;
  HttpParser    parser_;
  HttpResponse  response_;
  std::fstream  file_;
  bool          drain_incoming_ = false;
  size_t        drained_bytes_ = 0;
};

#endif //CLIENTCONNECTION_HPP
