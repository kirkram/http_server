/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: klukiano <klukiano@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/16 12:55:06 by dshatilo          #+#    #+#             */
/*   Updated: 2024/10/14 14:24:35 by klukiano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Socket.hpp"
#include "Logger.h"

Socket::Socket(std::string& listen,
         std::string& name,
         std::string& max_size,
         StringMap& errors,
         LocationMap& locations) {
  if (!listen.empty()) {
    size_t colon = listen.find(':');
    address_ = listen.substr(0, colon);
    port_ = listen.substr(colon + 1);
  }
  if (name.empty())
    name = "localhost"; //Or leave it emty may be
  v_hosts_.emplace(name, VirtualHost(max_size, errors, locations));
  first_vhost_ = name;
}

void  Socket::AddVirtualHost(std::string& name,
                             std::string& max_size,
                             StringMap&   errors,
                             LocationMap& locations) {
  if (!v_hosts_.contains(name))
    v_hosts_.emplace(name, VirtualHost(max_size, errors, locations));
}

int Socket::InitServer(std::vector<pollfd> &pollFDs) {
  struct addrinfo hints{}, *servinfo;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo(address_.c_str(), port_.c_str(), &hints, &servinfo);
  if (status  != 0) {
    logError(gai_strerror(status));
    return 1;
  }

  if ((listening_.fd = socket(servinfo->ai_family, servinfo->ai_socktype,
      servinfo->ai_protocol)) == -1) {
    logError("server: socket() error");
    return 2;
  }
  /* SO_REUSEADDR for TCP to handle the case when the server shuts down
    and we can't bind to the same socket if there's data left
    port goes into a TIME_WAIT state otherwise*/
  int yes = 1;
  if (setsockopt(listening_.fd, SOL_SOCKET, SO_REUSEADDR, &yes,
      sizeof(int)) == -1) {
    logError("server: setsockopt() error");
    return 3;
  }

  if (bind(listening_.fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    close(listening_.fd);
    logError("server: bind() error");
    return 4;
  }
  freeaddrinfo(servinfo);
  
  #define BACKLOG 10
  /* TODO: DEFINED IN THE BACKLOG PARAMETER */
  if (listen(listening_.fd, BACKLOG) == -1) {
    logError("server: listen() error");
    return 5;
  }
  listening_.events = POLLIN;
  pollFDs.push_back(listening_);
  return 0;
}

VirtualHost*  Socket::FindVhost(const std::string& host) {
  if (host.size() == 0 )
    return &v_hosts_.at(first_vhost_); //substitute first_vhost_ with pointer to VH if possible (pointer invalidation?)
  if (auto it = v_hosts_.find(host); it == v_hosts_.end())
    return &it->second;
  else
    return &v_hosts_.at(first_vhost_);
}

std::string Socket::ToString() const {
  std::string out;
  out += std::string(7, ' ') + "Address: " + address_ + "\n";
  out += std::string(7, ' ') + "Port: " + port_ + "\n";
  for (const auto& [name, vh] : v_hosts_) {
    out += std::string(7, ' ') + "Virtual host: " + name + "\n";
    out += vh.ToString() + "\n";
  }
  return out;
}

std::string Socket::getSocket() const{
  return address_ + ":" + port_;
}