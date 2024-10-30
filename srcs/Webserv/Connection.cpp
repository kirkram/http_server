/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By:  dshatilo < dshatilo@student.hive.fi >     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/17 10:16:03 by dshatilo          #+#    #+#             */
/*   Updated: 2024/10/30 23:52:09 by  dshatilo        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"

#include <unistd.h>

Connection::Connection(int fd, size_t timeout)
    : fd_(fd),
      last_active_(std::chrono::steady_clock::now()),
      timeout_(timeout) {}

Connection::~Connection() {}

bool Connection::HasTimedOut() const {
  timepoint now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<sec>(now - last_active_);
  return duration.count() >= timeout_;
}

void  Connection::UpdateLastActive() {
  last_active_ = std::chrono::steady_clock::now();
}