/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vsavolai <vsavolai@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/09 12:16:12 by vsavolai          #+#    #+#             */
/*   Updated: 2024/10/18 09:42:18 by vsavolai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef HTTPPARSER_HPP_
# define HTTPPARSER_HPP_

# include <iostream>
# include <map>
# include <sstream>
# include <string>
# include <algorithm>
# include <filesystem>
# include <unistd.h>
# include <fcntl.h>
# include <vector>
# include <array>
# include "VirtualHost.hpp"

#define MAXBYTES 8192

class VirtualHost;

class HttpParser {
 public:
  HttpParser(std::string& status);
  HttpParser(const HttpParser& other)             = delete;
  HttpParser& operator=(const HttpParser& other)  = delete;

  ~HttpParser() = default;

  bool        ParseHeader(const std::string& buffer);
  bool        HandleRequest(VirtualHost* vhost);
  int         WriteBody(VirtualHost* vhost,  std::vector<char>& buffer,
                        int bytesIn);
  bool        IsBodySizeValid(VirtualHost* vhost);
  void        ResetParser();
  std::string getHost() const;
  std::string getMethod() const;
  std::string getResourceTarget() const;
  std::string getFileList() const;

 private:
  bool  ParseStartLine(std::istringstream& request_stream);
  bool  ParseHeaderFields(std::istringstream& request_stream);
  bool  CheckPostHeaders();
  bool  UnChunkBody(std::vector<char>& buf);
  void  AppendBody(std::vector<char> buffer, int bytesIn);
  void  HandlePostRequest(std::vector<char> request_body);
  bool  HandleMultipartFormData(const std::vector<char> &body,
                                const std::string &contentType);
  bool  ParseMultiPartData(std::vector<char> &bodyPart);
  bool  ParseUrlEncodedData(const std::vector<char>& body);
  bool  IsPathSafe(const std::string& path);
  void  HandleDeleteRequest();
  void  GenerateFileListHtml();
  bool  CheckValidPath(std::string root);
  void  CreateDirListing(std::string directory);

  std::string&                        status_;
  size_t                              content_length_ = 0;
  std::string                         method_;
  std::string                         request_target_;
  std::string                         query_string_;
  std::string                         file_list_;
  std::string                         index_;
  std::vector<char>                   request_body_;
  std::map<std::string, std::string>  headers_;
  bool                                is_chunked_ = false;
};

#endif