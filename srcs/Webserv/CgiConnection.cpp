/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiConnection.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dshatilo <dshatilo@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/18 14:07:41 by dshatilo          #+#    #+#             */
/*   Updated: 2024/11/04 17:02:55 by dshatilo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fcntl.h>
#include <unistd.h>
#include "CgiConnection.hpp"
#include "Logger.hpp"
#include <signal.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

CgiConnection::CgiConnection(int read_fd,
                             int write_fd,
                             ClientConnection& client,
                             pid_t child_pid)
    : Connection(write_fd, 5),
      client_(client),
      file_(client.file_),
      child_pid_(child_pid) {
  pipe_fd_[READ] = read_fd;
  pipe_fd_[WRITE] = write_fd;
}

CgiConnection::~CgiConnection() {
  if (pipe_fd_[WRITE] != -1)
    close(pipe_fd_[WRITE]);
  close(pipe_fd_[READ]);
  client_.stage_ = ClientConnection::Stage::kResponse;

  int status;
  pid_t result = waitpid(child_pid_, &status, WNOHANG);

  if (result == -1) {
    kill(child_pid_, SIGTERM);
    client_.status_ = "500";
    return;
  }
  if (HasTimedOut()) {
    if (result == 0)
      kill(child_pid_, SIGTERM);
    client_.status_ = "504";
  } else if (client_.status_ != "200") {
    if (result == 0)
      kill(child_pid_, SIGTERM);
  } else if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) ||
              WIFSIGNALED(status)) {
    client_.status_ = "500";
  } else if (client_.status_ == "200") {
    client_.additional_headers_.insert(additional_headers_.begin(),
                                       additional_headers_.end());
    return;
  }
  file_.close();
  file_.open(client_.vhost_->getErrorPage(client_.status_));
}

pid_t  CgiConnection::CreateCgiConnection(ClientConnection& client) {
  int to_cgi_pipe[2] = {-1, -1};
  int from_cgi_pipe[2] = {-1, -1};

  if (pipe(to_cgi_pipe) == -1) {
    logError("CGI: failed to pipe().");
    return -1;
  }
  if (pipe(from_cgi_pipe) == -1) {
    close(to_cgi_pipe[READ]);
    close(to_cgi_pipe[WRITE]);
    logError("CGI: failed to pipe().");
    return -1;
  }
  if (fcntl(to_cgi_pipe[WRITE], F_SETFL, O_NONBLOCK) == -1 || 
      fcntl(from_cgi_pipe[READ], F_SETFL, O_NONBLOCK) == -1 ) {
    close(to_cgi_pipe[READ]);
    close(to_cgi_pipe[WRITE]);
    close(from_cgi_pipe[READ]);
    close(from_cgi_pipe[WRITE]);
    logError("CGI: failed to fcnt().");
    return -1;
  }
  pid_t pid = fork();
  if (pid == -1) {
    close(to_cgi_pipe[READ]);
    close(to_cgi_pipe[WRITE]);
    close(from_cgi_pipe[READ]);
    close(from_cgi_pipe[WRITE]);
    logError("CGI: failed to fork().");
    return -1;
  }
  if (pid != 0) {
    close(to_cgi_pipe[READ]);
    close(from_cgi_pipe[WRITE]);
    pollfd cgi_poll = {to_cgi_pipe[WRITE], POLLOUT, 0};
    client.webserv_.SwitchClientToSend(client.fd_);
    client.webserv_.AddNewConnection(
        cgi_poll,
        std::make_unique<CgiConnection>(from_cgi_pipe[READ],
                                        to_cgi_pipe[WRITE],
                                        client, pid));
  }
  else {
    close(to_cgi_pipe[WRITE]);
    close(from_cgi_pipe[READ]);
    if (chdir("cgi-bin") == -1)
      std::_Exit(EXIT_FAILURE);
    StartCgiProcess(to_cgi_pipe[READ], from_cgi_pipe[WRITE], client);
  }
  return pid;
}

void  CgiConnection::StartCgiProcess(int read_fd,
                                     int write_fd,
                                     ClientConnection& client) {
  if (dup2(read_fd, STDIN_FILENO) == -1 ||
      dup2(write_fd, STDOUT_FILENO) == -1) {
    close(read_fd);
    close(write_fd);
    std::_Exit(EXIT_FAILURE);
  }
  close(read_fd);
  close(write_fd);

  std::vector<std::string> env_vec = client.PrepareCgiEvniron();
  std::vector<char*>env;
  env.reserve(env_vec.size() + 1);
  for (size_t i = 0; i < env_vec.size(); ++i)
    env[i] = env_vec[i].data();
  env[env_vec.size()] = nullptr;

  std::string target = client.parser_.getRequestTarget().data() + 9;
  std::string executable;
  char* cmd[3];

  if (target.ends_with(".py")) {
    executable = "/usr/bin/python3";
    cmd[0] = executable.data();
    cmd[1] = target.data();
  } else if (target.ends_with(".php")) {
    executable = "/usr/bin/php";
    cmd[0] = executable.data();
    cmd[1] = target.data();
  } else {
    cmd[0] = target.data();
    cmd[1] = nullptr;
  }
  cmd[2] = nullptr;
  execve(cmd[0], cmd, env.data());
  std::_Exit(EXIT_FAILURE);
}

int CgiConnection::ReceiveData(pollfd& poll) {
  char  buffer[MAXBYTES] = {0};
  int   bytes_in;

  bytes_in = read(poll.fd, buffer, MAXBYTES);
  if (bytes_in == -1) {
    logError("CGI: failed to read from child process.");
    return 1;
  }
  if (bytes_in == 0) {
    logInfo("CGI: finished reading from child.");
    file_.seekg(0);
    return 1;
  }
  if (!header_parsed_) {
    header_parsed_ = ParseCgiResponseHeaderFields(buffer);
    if (!header_parsed_) {
      client_.status_ = "502";
      return 1;
    }
  } else
    file_.write(buffer, bytes_in);
  return 0;
}

int CgiConnection::SendData(pollfd& poll) {
  char  buffer[MAXBYTES];

  if (!file_.is_open())
    return SwitchToReceive();

  file_.read(buffer, MAXBYTES);
  if (file_.fail() && !file_.eof()) {
    logError("CGI: failed to read from fstream.");
    client_.status_ = "500";
    return 1;
  }
  if (file_.gcount() == 0 && file_.eof())
    return SwitchToReceive();
  if (write(poll.fd, buffer, file_.gcount()) == -1) {
    logError("CGI: failed to write data to child process.");
    client_.status_ = "500";
    return 1;
  }
  return 0;
}

int CgiConnection::SwitchToReceive() {
  client_.webserv_.SwitchCgiToReceive(pipe_fd_[WRITE], pipe_fd_[READ]);
  fd_ = pipe_fd_[READ];
  pipe_fd_[WRITE] = -1;

  file_.close();
  std::string filename = "/tmp/webserv/cgi_"  + std::to_string(fd_);
  file_.open(filename,
             std::fstream::in |
             std::fstream::out |
             std::fstream::trunc |
             std::ios::binary);
  if (!file_.is_open()) {
    logError("CGI: failed to open temporary response file for child process.");
    client_.status_ = "500";
    return 1;
  }
  std::remove(filename.c_str());
  return 0;
}

bool CgiConnection::ParseCgiResponseHeaderFields(char* buffer) {
  std::stringstream  response_stream(buffer);
  std::string         line;
  while (std::getline(response_stream, line) && line != "\r") {
    size_t delim = line.find(":");
    if (delim == std::string::npos || line.back() != '\r') {
      logError("CGI: Wrong header line format.");
      client_.status_ = "502";
      return false;
    }
    line.pop_back();
    std::string header = line.substr(0, delim + 1);
    std::string header_value = line.substr(delim + 1);
    additional_headers_[header] = header_value;
  }

  if (!additional_headers_.contains("Content-Type:")) {
    logError("CGI: failed to find \"Content-Type\" in headers.");
    client_.status_ = "502";
    return false;
  }

  if (line != "\r") {
    logError("CGI: Response header fields too large");
    client_.status_ = "502";
    return false;
  }

  if (response_stream.rdbuf()->in_avail() != 0)
    file_ << response_stream.rdbuf();

  return true;
}
