/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientInfo.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: klukiano <klukiano@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 17:38:49 by klukiano          #+#    #+#             */
/*   Updated: 2024/10/10 17:39:42 by klukiano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ClientInfo_HPP
#define ClientInfo_HPP

#include "HttpParser.hpp"
#include "HttpResponse.hpp"
#include <poll.h>
#include <fstream>

class Socket;
class VirtualHost;

class ClientInfo
{
 public:
  ClientInfo();
  // ClientInfo(int fd, Socket* sock);
  ClientInfo(const ClientInfo& other)             = delete;
  ClientInfo& operator=(const ClientInfo& other)  = delete;
  ~ClientInfo()                                   = default;

  void            InitInfo(int fd, Socket *sock);
  int             RecvRequest(pollfd& poll);
  HttpParser&     getParser();
  HttpResponse&   getResponse();
  VirtualHost*    getVhost();
  int             getFd();
  std::ifstream&  getGetfile();
  bool            getIsSending();
  void            setIsSending(bool boolean);

private:
  int            status_ = 200;
  int            fd_;
  Socket*        sock_ = nullptr;
  VirtualHost*   vhost_ = nullptr;
  HttpParser     parser_;
  HttpResponse   response_;
  std::ifstream  getfile_;
  bool           is_sending_chunks_ = false;
  bool           is_parsing_body_ = false;
};

#endif