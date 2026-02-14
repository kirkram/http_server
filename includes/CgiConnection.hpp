#ifndef CGICONNECTION_HPP
#define CGICONNECTION_HPP

#include "Connection.hpp"
#include "ClientConnection.hpp"
#include "WebServ.hpp"

#include <fstream>
#include <set>

class CgiConnection : public Connection {
 private:
  using StringSet = std::set<std::string>;

 public:
  CgiConnection(int read_fd,
                int write_fd,
                ClientConnection& client,
                pid_t child_pid);
  CgiConnection(const CgiConnection& other)             = delete;
  CgiConnection& operator=(const CgiConnection& other)  = delete;

  ~CgiConnection() override;

  static pid_t  CreateCgiConnection(ClientConnection& client);
  int           ReceiveData(pollfd& poll) override;
  int           SendData(pollfd& poll) override;

 private:
  static void             StartCgiProcess(int read_fd, int write_fd,
                                          ClientConnection& client);
  static const StringSet& getContentTypeSet();
  static const StringSet& getStatusSet();
  int                     SwitchToReceive();
  bool                    ParseCgiResponseHeaderFields(char* buffer);

  int               pipe_fd_[2];
  ClientConnection& client_;
  std::fstream&     file_;
  pid_t             child_pid_;
  bool              header_parsed_ = false;
};

#endif //CGICONNECTION_HPP