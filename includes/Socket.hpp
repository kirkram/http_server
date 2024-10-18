/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: klukiano <klukiano@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/02 15:28:14 by dshatilo          #+#    #+#             */
/*   Updated: 2024/10/14 14:24:19 by klukiano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include "VirtualHost.hpp"
#include "HttpParser.hpp"

#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <vector>
// #include <string.h>


class Socket {
 private:
  using StringMap = std::map<std::string, std::string>;
  using LocationMap = std::map<std::string, Location>;

 public:
  Socket(std::string& listen,
         std::string& name,
         std::string& max_size,
         StringMap& errors,
         LocationMap& locations);
  Socket(const Socket& other)            = default;
  Socket& operator=(const Socket& other) = delete;
  ~Socket()                              = default;

  void          AddVirtualHost(std::string& name,
                               std::string& max_size,
                               StringMap&   errors,
                               LocationMap& locations);
  int           InitServer(std::vector<pollfd> &pollFDs);
  VirtualHost*  FindVhost(const std::string& host);
  std::string   ToString() const;

  std::string                         getSocket() const;

 private:
  std::string                         address_ = "127.0.0.0"; //Is it correct?
  std::string                         port_ = "8080"; //Is it correct?
  pollfd                              listening_;
  std::map<std::string, VirtualHost>  v_hosts_;
  std::string                         first_vhost_;
};

#endif
