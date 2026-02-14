#ifndef LOCATION_HPP_
#define LOCATION_HPP_

#include <utility>
#include <string>

struct Location {
 public:
  using StringPair = std::pair<std::string, std::string>;

  Location(std::string& methods,
           StringPair& redirection,
           std::string& root,
           std::string& autoindex,
           std::string& index,
           std::string& upload);
  Location(const Location& other)             = default;
  Location(Location&& other)                  = default;
  Location& operator=(const Location& other)  = delete;
  Location& operator=(Location& other)        = delete;

  ~Location() = default;

  std::string ToString() const;

  std::string methods_;
  StringPair  redirection_;
  std::string root_;
  bool        autoindex_ = false;
  std::string index_;
  std::string upload_;
};

#endif
