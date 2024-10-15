/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientInfo.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dshatilo <dshatilo@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 17:38:49 by klukiano          #+#    #+#             */
/*   Updated: 2024/10/15 11:48:10 by dshatilo         ###   ########.fr       */
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
  ClientInfo(int fd, Socket& sock);
  ClientInfo(const ClientInfo& other)             = delete;
  ClientInfo& operator=(const ClientInfo& other)  = delete;
  ClientInfo(ClientInfo&& other);
  ClientInfo& operator=(ClientInfo&& other)       = delete;
  ~ClientInfo()                                   = default;

  int             RecvRequest(pollfd& poll);
  void            SendResponse(pollfd& poll);
  void            ResetClientInfo();
  HttpParser&     getParser();
  VirtualHost*    getVhost();
  int             getFd();
  std::ifstream&  getGetfile();
  bool            getIsSending();
  void            setIsSending(bool boolean);

private:
  std::string    status_ = "200";
  int            fd_;
  Socket&        sock_;
  VirtualHost*   vhost_ = nullptr;
  HttpParser     parser_;
  HttpResponse   response_;
  std::ifstream  getfile_;
  bool           is_sending_chunks_ = false;
  bool           is_parsing_body_ = false;
};

#endif