
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServ.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: klukiano <klukiano@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/06 15:30:31 by dshatilo          #+#    #+#             */
/*   Updated: 2024/09/30 17:16:35 by klukiano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include "ConfigParser.hpp"
#include "WebServ.hpp"

#define TODO 123

WebServ::WebServ(const char* conf)
    : conf_(conf != nullptr ? conf : DEFAULT_CONF) {
  if (conf == nullptr)
    logInfo("Configuration file not provided.\n\t\t\t     Using the "
            "default configuration file: /" + std::string(DEFAULT_CONF));
}

int WebServ::Init() {
  {
    ConfigParser parser(conf_);
    if (parser.ParseConfig(this->sockets_))
      return 1;
  }
  logDebug(ToString(), true);
  for (Socket& socket : sockets_) {
    if (socket.InitServer(pollFDs_))
      return 2;
    logDebug("init the server on socket " + socket.getSocket());
  }
  logInfo("Servers are ready");
  return (0);
}

#define TIMEOUT   5000

void WebServ::Run() {
  int socketsReady = 0;
  while (true) {
    /* the socketsReady from poll() 
      is the number of file descriptors with events that occurred. */
    socketsReady = poll(pollFDs_.data(), pollFDs_.size(), TIMEOUT);
    if (socketsReady == -1)
      perror("poll: ");
    else if (!socketsReady) {
      /* TODO: individual timer for the timeout close */
      logInfo("poll() is closing connections on timeout...");
      while (pollFDs_.size() != sockets_.size()) {
        close(pollFDs_.back().fd);
        pollFDs_.pop_back();
      }
      client_info_map_.clear();
    } else
        PollAvailableFDs();
  }
  CloseAllConnections();
  logInfo("--- Shutting down the server ---");
}

void WebServ::PollAvailableFDs(void) {
  for (size_t i = 0; i < pollFDs_.size(); i++) {
    int fd = pollFDs_[i].fd;
    short revents = pollFDs_[i].revents;
    if (i < sockets_.size()) {
      /* Don't accept new connections if the num of clients is a certain arbitrary value */
      if (pollFDs_.size() < 1000 + sockets_.size())
        CheckForNewConnection(fd, revents, i);
      continue;
    }
    ClientInfo& fd_info = client_info_map_.at(fd);
    if (revents & POLLERR) {
      logDebug("error or read end has been closed", true);
      CloseConnection(fd, i);
    } else if (revents & POLLHUP) { 
        logDebug("Hang up: " + std::to_string(fd), true);
        CloseConnection(fd, i);
    } else if (revents & POLLNVAL) {
        logDebug("Invalid fd: " + std::to_string(fd));
        CloseConnection(fd, i);
    } else if (revents & POLLIN)
        RecvFromClient(fd_info, i);
      else if (revents & POLLOUT)
        SendToClient(fd_info, pollFDs_[i]);
  }
}

void WebServ::CheckForNewConnection(int fd, short revents, int i) {
  if (revents & POLLIN) {
    pollfd new_client;
    new_client.fd = accept(fd, nullptr, nullptr);
    if (new_client.fd != -1) {
      /* O_NONBLOCK: No I/O operations on the file descriptor will cause the
            calling process to wait. */
      fcntl(new_client.fd, F_SETFL, O_NONBLOCK);
      new_client.events = POLLIN;
      pollFDs_.push_back(new_client);
      client_info_map_.emplace(new_client.fd, ClientInfo(new_client.fd,
                                                         sockets_[i]));
    }
    else 
      logError("CheckForNewConnection: accept() failed");
  }
}

void WebServ::RecvFromClient(ClientInfo& fd_info, size_t& i) {
  if (fd_info.RecvRequest(pollFDs_[i]))
    CloseConnection(pollFDs_[i].fd, i);
}

void WebServ::SendToClient(ClientInfo& fd_info, pollfd& poll) {
  fd_info.SendResponse(poll);
  if (poll.events == POLLIN)
    fd_info.ResetClientInfo();
}

void WebServ::CloseConnection(int sock, size_t& i) {
  close(sock);
  pollFDs_.erase(pollFDs_.begin() + i);
  client_info_map_.erase(sock);
  i--;
}

void WebServ::CloseAllConnections(void) {
  for (size_t i = 0; i < pollFDs_.size(); i++)
    close(pollFDs_[i].fd);
}

std::string WebServ::ToString() const {
  std::string out("***Webserv configuration***\n");
  out += "Configuuration file used: " + conf_ + "\n";
  for (const auto& socket : sockets_) {
    out += "Server\n";
    out += socket.ToString() + "\n";
  }
  return out;
}
