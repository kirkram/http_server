#ifndef HTTPRESPONSE_HPP_
#define HTTPRESPONSE_HPP_

#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <poll.h>
#include <vector>
#include "Location.hpp"

# define MAXBYTES 8192

class ClientConnection;
class HttpParser;

class HttpResponse {

 private:
  using StringMap = std::map<std::string, std::string>;

 public:
  HttpResponse(ClientConnection& client);
  HttpResponse(const HttpResponse& other)             = delete;
  HttpResponse& operator=(const HttpResponse& other)  = delete;
  ~HttpResponse() = default;

  void  PrepareResponse();
  int   SendResponse(pollfd &poll);
  void  ResetResponse();

 private:
  void  AssignContType();
  void  ComposeHeader();
  int   SendBuffer(int fd);
  int   SendHeader(int fd);
  int   SendBody(int fd);
  int   getOneChunk(std::fstream &file);
  int   SendToClient(const int clientSocket, const char *msg, int length);

  const StringMap& getContTypeMap();
  const StringMap& getStatusMap();

  ClientConnection& client_;
  std::string       header_;
  std::string       status_message_;
  bool              header_sent_ = false;
  bool              body_sent_ = false;
  char              buffer_[MAXBYTES];
  int               buffer_length_ = 0;
};

#endif