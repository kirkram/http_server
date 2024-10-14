/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   VirtualHost.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: klukiano <klukiano@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/15 17:35:52 by  dshatilo         #+#    #+#             */
/*   Updated: 2024/10/14 18:51:11 by klukiano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/stat.h>
#include "VirtualHost.hpp"
#include "ClientInfo.hpp"
#include "Logger.h"
#include <string>

#define MAXBYTES  8192

VirtualHost::VirtualHost(std::string& max_size,
                         StringMap& errors,
                         LocationMap& locations)
    : locations_(locations) {
  if (!max_size.empty()) {
    client_max_body_size_ = std::stoi(max_size);
    client_max_body_size_ *= (max_size.back() == 'M' ? 1048576 : 1024);
  }
  for (const auto& [key, value] : errors) {
    if (access(value.c_str(), R_OK) == -1)
      continue;
    error_pages_[key] = value;
  }
}

std::string VirtualHost::ToString() const {
  std::string out;
  out += std::string(21, ' ') + "Error Pages:\n";
  for (const auto& [code, path] : error_pages_)
    out += std::string(34, ' ') + "Error " + code + ": " + path + "\n";
  out += std::string(21, ' ') + "Client_max_body_size: ";
  out += std::to_string(client_max_body_size_) + " bytes\n";
  for (const auto& [path, location] : locations_) {
    out += std::string(21, ' ') + "Location: " + path + "\n";
    out += location.ToString() + "\n";
  }
  return out;
}

size_t VirtualHost::getMaxBodySize() const {
  return client_max_body_size_;
}

std::string VirtualHost::getErrorPage(int error) const {
  std::cout << "here" << std::endl;
  auto it = error_pages_.find(std::to_string(error));
  
  if (it != error_pages_.end())
    return it->second;
  else {
    logError("getErrorPage: can't find the error page, returning 404");
    it = error_pages_.find("404");
    if (it != error_pages_.end())
      return it->second;
    else
      return ("/www/404.html");
  }
}