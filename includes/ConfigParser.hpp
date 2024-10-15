/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By:  dshatilo < dshatilo@student.hive.fi >     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/19 15:56:49 by klukiano          #+#    #+#             */
/*   Updated: 2024/10/14 21:32:53 by  dshatilo        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP_
#define CONFIGPARSER_HPP_

#include "Logger.h"
#include "Location.hpp"
#include "VirtualHost.hpp"
#include "Socket.hpp"

#include <algorithm>
#include <map>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <utility>


class ConfigParser
{
 public:
  ConfigParser(const std::string& conf);
  ~ConfigParser() = default;

  int ParseConfig(std::deque<Socket>& sockets_);

 private:
  using StringMap = std::map<std::string, std::string>;
  using LocationMap = std::map<std::string, Location>;
  using StringPair = std::pair<std::string, std::string>;

  void ParseServer(std::stringstream& ss, std::deque<Socket> &sockets_);
  void ParseListen(std::string& s, std::stringstream& ss);
  void ParseServerName(std::string& name, std::stringstream& ss);
  void ParseMaxBodySize(std::string& max_size, std::stringstream& ss);
  void ParseErrorPage(StringMap& errors, std::stringstream& ss);
  void ParseLocation(LocationMap& locations, std::stringstream& ss);
  void ParseAllowedMethods(std::string& methods, std::stringstream& ss);
  void ParseRedirection(StringPair& redirection, std::stringstream& ss);
  void ParseRoot(std::string& root, std::stringstream& ss);
  void ParseAutoindex(std::string& autoindex, std::stringstream& ss);
  void ParseIndex(std::string& index, std::stringstream& ss);
  void ParseUpload(std::string& upload, std::stringstream& ss);

  std::ifstream conf_;
};

#endif

