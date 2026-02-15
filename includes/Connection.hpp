#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <chrono>
#include <iostream>
#include <cstddef>
#include <poll.h>
#include <map>


class Connection
{
 private:
 using timepoint = std::chrono::steady_clock::time_point;
 using sec = std::chrono::seconds;
 using StringMap = std::map<std::string, std::string>;

 public:
  Connection(int fd, size_t timeout);
  Connection(const Connection& other) = delete;
  Connection(Connection&& other)      = default;
  Connection& operator=(const Connection& other) = delete;
  Connection& operator=(Connection&& other)      = delete;

  virtual ~Connection();

  virtual int   ReceiveData(pollfd& poll) = 0;
  virtual int   SendData(pollfd& poll)    = 0;
  bool          HasTimedOut() const;
  void          UpdateLastActive();

  int            fd_;
  timepoint      last_active_;
  StringMap      additional_headers_;

 protected:
  const long     timeout_;
};

#endif //CONNECTION_HPP