/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dshatilo <dshatilo@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/16 11:49:17 by dshatilo          #+#    #+#             */
/*   Updated: 2024/10/15 13:11:25 by dshatilo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Location.hpp"

Location::Location(std::string& methods,
                   StringPair&  redirection,
                   std::string& root,
                   std::string& autoindex,
                   std::string& index,
                   std::string& upload)
    : methods_(methods),
      redirection_(redirection),
      root_(root),
      autoindex_(autoindex == "on" ? true : false),
      index_(index),
      upload_(upload) {}

std::string Location::ToString() const {
  std::string out;
  out += std::string(31, ' ') + "Allowed methods: ";
  if (methods_.find("GET") != std::string::npos)
    out += "GET ";
  if (methods_.find("HEAD") != std::string::npos)
    out += "HEAD ";
  if (methods_.find("POST") != std::string::npos)
    out += "POST ";
  if (methods_.find("DELETE") != std::string::npos)
    out += "DELETE ";
  out += "\n";
  if (!redirection_.first.empty()) {
    out += std::string(31, ' ') + "Redirection: ";
    out += redirection_.first + " " + redirection_.second + "\n";
  }
  if (!root_.empty()) {
    out += std::string(31, ' ') + "Root: " + root_ + "\n";
  }
  out += std::string(31, ' ') + "Autoindex: ";
  out += autoindex_ ? "on\n" : "off\n";
  if (!index_.empty()) {
    out += std::string(31, ' ') + "Index: " + index_ + "\n";;
  }
  if (!upload_.empty()) {
    out += std::string(31, ' ') + "Upload directory: " + upload_;
  }
  return out;
}
