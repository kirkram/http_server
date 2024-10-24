/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   VirtualHost.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: klukiano <klukiano@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/15 17:35:52 by  dshatilo         #+#    #+#             */
/*   Updated: 2024/10/24 18:07:29 by klukiano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/stat.h>
#include "VirtualHost.hpp"
#include "ClientConnection.hpp"
#include "Logger.hpp"
#include <string>

#define MAXBYTES  8192

VirtualHost::VirtualHost(std::string& max_size,
                         StringMap& errors,
                         LocationMap& locations)
    : error_pages_(getDefaultErrorPages()),
      locations_(locations) {
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

std::map<std::string, Location>& VirtualHost::getLocations() {
    return locations_;
}

std::string VirtualHost::getErrorPage(const std::string& error) const {
  auto it = error_pages_.find(error);
  return it->second;
}

const VirtualHost::StringMap& VirtualHost::getDefaultErrorPages() {
  static const StringMap error_pages = {{"400", "www/error_pages/400.html"},
                                        {"403", "www/error_pages/403.html"},
                                        {"404", "www/error_pages/404.html"},
                                        {"405", "www/error_pages/405.html"},
                                        {"411", "www/error_pages/411.html"},
                                        {"413", "www/error_pages/413.html"},
                                        {"415", "www/error_pages/415.html"},
                                        {"431", "www/error_pages/431.html"},
                                        {"500", "www/error_pages/500.html"},
                                        {"501", "www/error_pages/501.html"},
                                        {"502", "www/error_pages/502.html"},
                                        {"504", "www/error_pages/504.html"},
                                        {"505", "www/error_pages/505.html"}};
  return error_pages;
}
