/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServ.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By:  dshatilo < dshatilo@student.hive.fi >     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/05 15:17:45 by shatilovdr        #+#    #+#             */
/*   Updated: 2024/10/08 22:44:22 by  dshatilo        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP_
#define WEBSERV_HPP_

#include <netdb.h> 
#include <unistd.h>
#include <poll.h>
#include <deque>
#include "Socket.hpp"
#include "ClientInfo.hpp"

#define DEFAULT_CONF "conf/default.conf"

class WebServ {
 public:
  WebServ(const char* config_file);
  WebServ(const WebServ& other)            = delete;
  WebServ& operator=(const WebServ& other) = delete;

  ~WebServ() = default;

  int   Init();
  void  Run();

 private:
  const std::string         conf_;
  std::deque<Socket>        sockets_;
  std::vector<pollfd>       pollFDs_;
  std::map<int, ClientInfo> client_info_map_;

  void        PollAvailableFDs(void);
  void        CheckForNewConnection(int fd, short revents, int i);
  void        RecvFromClient(ClientInfo& fd_info, size_t& i);
  void        SendToClient(ClientInfo& fd_info, pollfd& poll);
  void        CloseConnection(int sock, size_t& i);
  void        CloseAllConnections(void);
  std::string ToString() const;

  struct EventFlag {
      short flag;
      const char* description;
  };
};
#endif
