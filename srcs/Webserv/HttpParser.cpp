/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dshatilo <dshatilo@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/09 13:13:54 by vsavolai          #+#    #+#             */
/*   Updated: 2024/10/15 11:56:05 by dshatilo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpParser.hpp"
#include "Logger.h"

HttpParser::HttpParser(std::string& status) : status_(status) {}

bool HttpParser::ParseHeader(const std::string& request) {
  std::istringstream  request_stream(request);

  if (!ParseStartLine(request_stream))
    return false;
  if (!ParseHeaderFields(request_stream))
    return false;
  if (method_ == "POST" && !CheckPostHeaders())
    return false;
  else if (method_ == "DELETE") //CHECKS FOR DELETE METHOD IF NEEDED                  
    ;
  else
    return true;
  std::streampos current = request_stream.tellg();
  request_stream.seekg(0, std::ios::end); /* Beware of off by 1, should be good tho */
  size_t stream_size = request_stream.tellg() - current;
  request_stream.seekg(current);
  request_body_.reserve(stream_size);
  request_stream.read(request_body_.data(), stream_size);
  return true;
}

int HttpParser::WriteBody(VirtualHost* vhost, std::vector<char>& buffer,
                          int bytesIn) {
  std::string eoc = "0\r\n\r\n";

  if (bytesIn > 5 || !std::equal(buffer.begin(), buffer.end(), eoc.begin())) {
    logDebug("bytesIn == MAXBYTES, more data to recieve");
    AppendBody(buffer, bytesIn);
    if (!IsBodySizeValid(vhost) || request_body_.size() > content_length_) {
      logError("Error: Request Header Fields Too Large");
      status_ = "431";
      return 1;
    }
    return 0;
  }
  UnChunkBody(request_body_);
  return 1;
}

bool  HttpParser::IsBodySizeValid(VirtualHost* vhost) {
  if (request_body_.size() > vhost->getMaxBodySize()) {
    logError("Error: Request Header Fields Too Large");
    status_ = "431";
    return false;
  }
  return true;
}

void  HttpParser::ResetParser() {
  content_length_ = 0;
  method_.clear();
  request_target_.clear();
  query_string_.clear();
  request_body_.clear();
  headers_.clear();
  is_chunked_ = false;
}

std::string  HttpParser::getHost() const {
  return headers_.at("Host");
}

std::string HttpParser::getMethod() const {
  return method_;
}

std::string HttpParser::getResourceTarget() const {
  return request_target_;
}


bool  HttpParser::ParseStartLine(std::istringstream& request_stream) {
  std::string line;
  std::string http_version;
  std::getline(request_stream, line);
  std::istringstream line_stream(line);
  line_stream >> method_ >> request_target_ >> http_version;
  std::getline(line_stream, line);

  if (method_.empty() || request_target_.empty()
      || http_version.empty() || line != "\r") {
    logError("Error: Bad request 400");
    status_ = "400";
    return false;
  }

  static std::array<std::string, 4> allowed_methods = {
      "GET", "POST", "DELETE", "HEAD"};
  if (std::find(allowed_methods.begin(), allowed_methods.end(), method_) ==
      allowed_methods.end()) {
    logError("Error: not supported method requested");
    status_ = "501";
    return false;
  }

  if (request_target_[0] != '/') {
    logError("Error: Bad request 400");
    status_ = "400";
    return false;
  }

  if (http_version != "HTTP/1.1") {
    logError("Error: HTTP Version Not Supported 505");
    status_ = "505";
    return false;
  }

  size_t query_position = request_target_.find("?");
  if (query_position != std::string::npos) {
    query_string_ = request_target_.substr(query_position + 1);
    request_target_ = request_target_.substr(0, query_position);
  }
  return true;
}

bool  HttpParser::ParseHeaderFields(std::istringstream& request_stream) {
  std::string line;
  while (std::getline(request_stream, line) && line != "\r") {
    size_t delim = line.find(":");
    if (delim == std::string::npos || line.back() != '\r') {
      logError("Error: wrong header line format");
      status_ = "400";
      return false; 
    }
    line.pop_back();
    std::string header = line.substr(0, delim);
    std::string header_value = line.substr(delim + 1);
    headers_[header] = header_value;
  }
  if (!headers_.contains("Host")) {
    logError("Error: Bad request 400");
    status_ = "400";
    return false;
  }
  if (line != "\r") {
    logError("Error: Request Header Fields Too Large");
    status_ = "431";
    return false;
  }
  return true;
}

bool  HttpParser::CheckPostHeaders() {
  auto it = headers_.find("transfer-encoding");
  is_chunked_ = (it != headers_.end() && it->second == "chunked");
  if (!is_chunked_) {
    auto it = headers_.find("Content-Length");
    if (it == headers_.end()) {
      logError("Error: content-lenght missing for request body");
      status_ = "411";
      return false;
    } else {
      std::string& content_length_str = it->second;
      try {
        size_t pos;
        int content_length = std::stoi(content_length_str, &pos);
        if (pos != content_length_str.size() || content_length < 0)
          throw std::invalid_argument("Invalid argument");
        content_length_ = content_length;
      } catch (const std::invalid_argument& e) {
        logError("Error: invalid Content-Length");
        status_ = "400";
        return false;
      } catch (const std::out_of_range& e) {
        logError("Error: Content-Length out of range");
        status_ = "413";
        return false;
      }
    }
  }
  return true;
}

bool HttpParser::UnChunkBody(std::vector<char>& buf) {
  std::size_t readIndex = 0;
  std::size_t writeIndex = 0;

  while(readIndex < buf.size()) {
    std::size_t chunkSizeStart = readIndex;

    while (readIndex < buf.size() && !(buf[readIndex] == '\r'
            && buf[readIndex + 1] == '\n')) {
      readIndex++;
    }

    if (readIndex >= buf.size()) {
      logError("UnChunkBody: \\r\\n missing");
      status_ = "400";
      return false;
    }

    std::string chunkSizeStr(buf.begin() + chunkSizeStart,
                             buf.begin() + readIndex);
    std::size_t chunkSize;
    std::stringstream ss;
    ss << std::hex << chunkSizeStr;
    ss >> chunkSize;

    readIndex += 2;

    if (chunkSize == 0) {
      break;
    }

    if (readIndex + chunkSize > buf.size()) {
      logError("UnChunkBody: empty line missing");
      status_ = "400";
      return false;
    }

    for (std::size_t i = 0; i < chunkSize; ++i) {
      buf[writeIndex++] = buf[readIndex++];
    }

    if (buf[readIndex] == '\r' && buf[readIndex + 1] == '\n') {
      readIndex += 2;
    } else {
      status_ = "400";
      logError("UnChunkBody: \\r\\n missing");
      return false;
    }
  }
  buf.resize(writeIndex);
  return true;
}

void HttpParser::AppendBody(std::vector<char> buffer, int bytesIn) {
  request_body_.insert(request_body_.end(), buffer.begin(),
                       buffer.begin() + bytesIn);
}

void  HttpParser::HandlePostRequest(std::vector<char> request_body) {
  auto it = headers_.find("Content-Type");

  if (it == headers_.end())
  {
    status_ = "400";
    return;
  }

  std::string contentType = it->second;

  if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
        logDebug("Handling URL-encoded form submission");
        if (!ParseUrlEncodedData(request_body)) {
             status_ = "500"; // Internal Server Error
            return;
        }
        
    } else if (contentType.find("multipart/form-data") != std::string::npos) {
      logDebug("Handling multipart form data");
        if (!HandleMultipartFormData(request_body, contentType)) {
            status_ = "500"; // Internal Server Error
            return;
        }
    } else {
        logError("Unsupported Content-Type");
        status_ = "415";
    }
}

bool HttpParser::HandleMultipartFormData(const std::vector<char> &body, const std::string &contentType) { 
    size_t boundaryPosition = contentType.find("boundary=");
    if (boundaryPosition == std::string::npos)
        return false;

    std::string boundary = "--" + contentType.substr(boundaryPosition + 9);

    std::vector<char> boundaryVec(boundary.begin(), boundary.end());
    size_t boundaryLength = boundaryVec.size();

    auto pos = body.begin();

    while (true) {
        auto boundaryStart = std::search(pos, body.end(), boundaryVec.begin(), boundaryVec.end());
        if (boundaryStart == body.end())
            break;

        pos = boundaryStart + boundaryLength;

        auto nextBoundaryStart = std::search(pos, body.end(), boundaryVec.begin(), boundaryVec.end());

        std::vector<char> bodyPart(pos, nextBoundaryStart);

        if (!ParseMultiPartData(bodyPart))
            return false;

        pos = nextBoundaryStart;
    }

    return true;
}

bool HttpParser::ParseMultiPartData(std::vector<char> &bodyPart) {
    std::vector<char> crlf = {'\r', '\n'};
    
    auto headerEndIt = std::search(bodyPart.begin(), bodyPart.end(), crlf.begin(), crlf.end());
    if (headerEndIt == bodyPart.end()) {
        logError("Error: invalid multi part format");
        return false;
    }

    std::vector<char> headers(bodyPart.begin(), headerEndIt);
    std::vector<char> content(headerEndIt + 2, bodyPart.end());

    std::string headersStr(headers.begin(), headers.end());

    if (headersStr.find("Content-Disposition") != std::string::npos) {
        size_t posFilename = headersStr.find("filename=");
        size_t posName = headersStr.find("name=");

        if (posFilename != std::string::npos) {

            size_t posStart = headersStr.find('"', posFilename) + 1;
            size_t posEnd = headersStr.find('"', posStart);
            std::string filename = headersStr.substr(posStart, posEnd - posStart);

            logDebug("File upload: " + filename);

            std::ofstream outFile("www/uploads/" + filename, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(content.data(), content.size());
                outFile.close();
                logDebug("File " + filename + " saved successfully");
            } else {
                logError("Error: failed to save file ");
                return false;
            }
        } else if (posName != std::string::npos) {
      
            size_t posStart = headersStr.find('"', posName) + 1;
            size_t posEnd = headersStr.find('"', posStart);
            std::string fieldName = headersStr.substr(posStart, posEnd - posStart);

            std::string contentStr(content.begin(), content.end());
            logDebug("Form field: " + fieldName + " = " + contentStr);
        }
    }

    return true;
}
  
bool HttpParser::ParseUrlEncodedData(const std::vector<char>& body) {
    std::string requestBody(body.begin(), body.end());
    std::string filename = "form_bin";

    std::istringstream stream(requestBody);
    std::string pair;

    std::ofstream outputFile(filename, std::ios::binary);
    if (!outputFile) {
        logError("Error: could not open file for writing.");
        return false;
    }

    while (std::getline(stream, pair, '&')) {
        size_t delim = pair.find('=');
        if (delim == std::string::npos) {
            logError("Error: URL encoding has wrong format.");
            return false;
        }
        std::string key = pair.substr(0, delim);
        std::string value = pair.substr(delim + 1);

        size_t keyLength = key.size();
        outputFile.write(reinterpret_cast<const char*>(&keyLength), sizeof(keyLength));
        outputFile.write(key.data(), keyLength);

        size_t valueLength = value.size();
        outputFile.write(reinterpret_cast<const char*>(&valueLength), sizeof(valueLength));
        outputFile.write(value.data(), valueLength);
    }

    outputFile.close();
    return true;
}

bool HttpParser::IsPathSafe(const std::string& path) {
  if (path.find(".."))
    return false;

    //this function we can add more checks to check safety
  return true;
}

void HttpParser::HandleDeleteRequest() {
    std::string path = request_target_;
    logDebug("Handling DELETE request for path: " + path);

    if (!IsPathSafe(path)) {
      status_ = "400";
      return;
    }

    if (std::ifstream(path)) { 
        if (std::remove(path.c_str()) == 0) {
            logDebug("File deleted successfully");
            status_ = "204";
        } else {
            logError("Error: Failed to delete file");
            status_ = "500";
        }
    } else {
        logError("Error: File not found");
        status_ = "404";
    }

}

// bool HttpParser::CheckValidPath(std::string path) {

//     status_ = "200";
//     /*for this function the root from confiq file is needed
//     in short this searches the asked path either directory or file
//     within the root directory*/
//     if (path.at(0) != '/') {
//         logError("Error: wrong path");
//         status_ = "404"; // or 400?
//         return false;
//     }

//     //untill we get root it has to be set manually for example "www" in current working directory
//     std::string rootPath = "";
//      try {
//         rootPath = std::filesystem::current_path().string() + "/www" + path;
//         logDebug("root path: " + rootPath);
//     } catch (const std::filesystem::filesystem_error& e) {
//         logError("Filesystem error: ");
//         std::cerr << e.what() << std::endl;
//         status_ = "500";
//         return false;
//     } catch (const std::exception& e) {
//         logError("Unexpected error: "); 
//         std::cerr << e.what() << std::endl;
//         status_ = "500";
//         return false;
//     }

//     /*check the directory and file existence and permissions, 
//     can take absolute and relative path, needs to be tested 
//     more when we get the root from confiq*/
//     try {
//         if (rootPath.back() == '/') {
//             if (std::filesystem::exists(rootPath) && std::filesystem::is_directory(rootPath)) {
//                 logDebug("valid path");
//                 return true;
//             } else {
//                 status_ = "404";
//                 logDebug("no valid path");
//                 return false;
//             }
//         } else {
//             if (std::filesystem::exists(rootPath) && std::filesystem::is_regular_file(rootPath)) {
//                 if (access(rootPath.c_str(), R_OK) == 0) {
//                     logDebug("file found");
//                     return true;
//                 } else {
//                     logDebug("permission denied");
//                     status_ = "403";
//                     return false;
//                 } 
//             } else {
//                 logDebug("file not found");
//                 status_ = "404";
//                 return false;
//             }
//         }
//     } catch (const std::filesystem::filesystem_error& e) {
//         logError("Filesystem error: ");
//         std::cerr << e.what() << std::endl;
//         status_ = "500";
//         return false;
//     } catch (const std::exception& e) {
//         logError("Unexpected error: "); 
//         std::cerr << e.what() << std::endl;
//         status_ = "500";
//         return false;
//     }

//     return true;
// }
