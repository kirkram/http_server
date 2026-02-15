
#include "ClientConnection.hpp"
#include "CgiConnection.hpp"
#include "Socket.hpp"
#include "Logger.hpp"

static const size_t kMaxDrainBytes = 1024 * 1024;

static void OpenErrorResponseFile(std::fstream& file,
                                  VirtualHost* vhost,
                                  const std::string& status) {
  if (file.is_open())
    file.close();
  file.clear();
  if (vhost == nullptr)
    return;
  std::string path = vhost->getErrorPage(status);
  if (path.empty())
    return;
  file.open(path, std::ios::in | std::ios::binary);
}

ClientConnection::ClientConnection(int fd, Socket& sock, WebServ& webserv)
    : Connection(fd, 10),
      sock_(sock),
      webserv_(webserv),
      parser_(*this),
      response_(*this) {}

ClientConnection::~ClientConnection() {
  close(fd_);
}

int ClientConnection::ReceiveData(pollfd& poll) {
  std::vector<char> buffer(MAXBYTES);
  int               bytesIn;
  bytesIn = recv(fd_, buffer.data(), MAXBYTES, 0);
  if (bytesIn < 0) {
    logError("Client ", poll.fd, ": recv returned -1, closing connection.");
    return 1;
  }
  if (bytesIn == 0) {
    logDebug("Client ", poll.fd, ": EOF received, closing connection.");
    return 2;
  }
  if (stage_ == Stage::kDrain) {
    drained_bytes_ += static_cast<size_t>(bytesIn);
    if (drained_bytes_ >= kMaxDrainBytes) {
      logInfo("Client ", poll.fd, ": drain limit reached, closing connection.");
      return 1;
    }
    return 0;
  }
  if (stage_ == Stage::kHeader) {
    bool header_parsed = parser_.ParseHeader(
        std::string(buffer.data(), static_cast<size_t>(bytesIn)));
    vhost_ = sock_.FindVhost(parser_.getHost());
    if (!header_parsed || !parser_.HandleRequest()) {
      OpenErrorResponseFile(file_, vhost_, status_);
      if (!file_.is_open()) {
        logError("Failed to open error page for status ", status_);
      }
      stage_ = Stage::kResponse;
    }
  }
  else if (stage_ == Stage::kBody) {
    bool body_read = parser_.WriteBody(buffer, bytesIn);
    if (!body_read) {
      logInfo("Body not read, trying to open error page - ", status_);
      if (status_ == "413")
        drain_incoming_ = true;
      OpenErrorResponseFile(file_, vhost_, status_);
      if (!file_.is_open()) {
        logError("Failed to open error page for status ", status_);
      }
      stage_ = Stage::kResponse;
    }
  }
  if (stage_ == Stage::kResponse)
    poll.events = POLLOUT;
  return 0;
}

int ClientConnection::SendData(pollfd& poll) {
  if (stage_ == Stage::kCgi)
    return 0;
  if (stage_ == Stage::kResponse) {
    response_.PrepareResponse();
    stage_ = Stage::kSending;
  }
  if (response_.SendResponse(poll)) {
    file_.close();
    logDebug("SendData returned 1, close");
    return 1;
  }
  if (poll.events == POLLIN) {
    if (status_ != "200") {
      if (drain_incoming_) {
        if (shutdown(fd_, SHUT_WR) == -1) {
          logDebug("Client ", fd_, ": shutdown(SHUT_WR) failed.");
        }
        drained_bytes_ = 0;
        stage_ = Stage::kDrain;
        return 0;
      }
      return 1;
    }
    ResetClientConnection();
  }
  return 0;
}

void  ClientConnection::ResetClientConnection() {
  status_ = "200";
  vhost_ = nullptr;
  parser_.ResetParser();
  response_.ResetResponse();
  additional_headers_.clear();
  file_.close();
  drain_incoming_ = false;
  drained_bytes_ = 0;
  stage_ = Stage::kHeader;
}

std::vector<std::string>  ClientConnection::PrepareCgiEvniron() {
  std::vector<std::string>  env;

  env.push_back("REQUEST_METHOD=" + parser_.method_);
  env.push_back("QUERY_STRING=" + parser_.query_string_);
  env.push_back("SCRIPT_NAME=" + parser_.request_target_);
  env.push_back("SERVER_NAME=" + vhost_->getName());
  env.push_back("CONTENT_TYPE=" + parser_.content_type_);
  env.push_back("CONTENT_LENGTH=" + std::to_string(parser_.content_length_));
  std::filesystem::path current_path = std::filesystem::current_path();
  env.push_back("PATH_INFO=" + current_path.generic_string());
  env.push_back("GATEWAY_INTERFACE=CGI/1.1");
  return env;
}
