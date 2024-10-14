/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   VirtualHost.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dshatilo <dshatilo@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/13 17:08:10 by  dshatilo         #+#    #+#             */
/*   Updated: 2024/10/14 10:47:21 by dshatilo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VIRTUALHOST_HPP_
#define VIRTUALHOST_HPP_

#include <iostream>
#include <string>
#include <map>
#include <utility>
#include <netdb.h> 
#include <poll.h>
#include <vector>

#include "Location.hpp"
#include "HttpResponse.hpp"
#include "HttpParser.hpp"

class VirtualHost {
 private:
  using StringMap = std::map<std::string, std::string>;
  using LocationMap = std::map<std::string, Location>;
  using StringPair = std::pair<std::string, std::string>;

 public:
  VirtualHost(std::string& max_size, StringMap& errors, LocationMap& locations);
  VirtualHost(const VirtualHost& other)             = default; //Why delete doesn't work?
  VirtualHost(VirtualHost&& other)                  = default;
  VirtualHost& operator=(const VirtualHost& other)  = delete;
  VirtualHost& operator=(VirtualHost&& other)       = delete;

  ~VirtualHost() = default;

  std::string ToString() const;
  size_t      getMaxBodySize() const;

 private:
  StringMap   error_pages_ = {{"404", "www/404.html"},
                              {"500", "www/500.html"}};
  size_t      client_max_body_size_ = 1048576;
  LocationMap locations_;
};

#endif
