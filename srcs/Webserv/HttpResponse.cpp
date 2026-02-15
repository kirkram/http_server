#include "HttpResponse.hpp"
#include "ClientConnection.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>

HttpResponse::HttpResponse(ClientConnection& client) : client_(client) {}

void HttpResponse::PrepareResponse() {
  std::string& status = client_.status_;
  if (status.compare(0, 1, "4") == 0 || status.compare(0, 1, "5") == 0) {
    client_.additional_headers_.clear();
    client_.additional_headers_["Content-Type:"] = "text/html";
    if (!client_.file_.is_open()) {
      status = "500";
      client_.additional_headers_["Content-Length:"] = "36";
    }
  }
  StringMap&  headers = client_.additional_headers_;
  if (auto it = headers.find("Content-Type:"); it == headers.end()) 
    AssignContType();

  if (auto it = headers.find("Status:"); it != headers.end()) {
    status_message_ = it->second;
    headers.erase(it);
  } else
    status_message_ = getStatusMap().find(client_.status_)->second;
  if (status.compare(0, 1, "3") == 0)
    body_sent_ = true;
  client_.additional_headers_["Server:"] = "miniserv-vsdskl";
  ComposeHeader();
  logDebug("Response prepared: " + status_message_);
  logDebug("Header prepared: " + header_);
}

void HttpResponse::AssignContType() {
  std::string request_target = client_.parser_.getRequestTarget();

  if (size_t delim = request_target.find_last_of('.'); delim != std::string::npos) {
    std::string extension = request_target.substr(delim);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    auto it = getContTypeMap().find(extension);
    if (it != getContTypeMap().end()) {
      client_.additional_headers_["Content-Type:"] = it->second;
      return;
    }
    logDebug("AssignContType: unknown extension: ", extension,
             " (request target: ", request_target, ")");
  }
  client_.additional_headers_["Content-Type:"] = "text/html";
}

void HttpResponse::ComposeHeader() {
  StringMap&         addhead = client_.additional_headers_;
  std::ostringstream oss;

  oss << "HTTP/1.1 " << status_message_ << "\r\n";
  for (auto it : addhead)
    oss << it.first << " " << it.second << "\r\n";

  if (addhead.find("Content-Length:") == addhead.end())
    oss << "Transfer-Encoding: chunked" << "\r\n";
  oss << "\r\n";
  this->header_ = oss.str();
}

int HttpResponse::SendResponse(pollfd& poll) {
  int             fd = client_.fd_;
  int             send_status = 0;

  if (buffer_length_ != 0)
    send_status = SendBuffer(fd);
  else if (!header_sent_)
    send_status = SendHeader(fd);
  else if (!body_sent_)
    send_status = SendBody(fd);
  if (send_status == -1)
    return 1;
  if (buffer_length_ == 0 && header_sent_ && body_sent_) {
    poll.events = POLLIN;
    logDebug("\n-----response sent-----\n");
  }
  return 0;
}

int HttpResponse::SendBuffer(int fd) {
  int send_status = SendToClient(fd, buffer_, buffer_length_);
  if (send_status == -1)
    return -1;
  if (send_status != buffer_length_) {
    buffer_length_ -= send_status;
    std::memcpy(buffer_, buffer_ + send_status, buffer_length_);
  } else
    buffer_length_ = 0;
  return 0;
}

int HttpResponse::SendHeader(int fd) {
  int len = std::min(header_.size(), static_cast<size_t>(MAXBYTES));
  int send_status = SendToClient(fd, header_.c_str(), len);
  if (send_status == -1)
    return -1;
  if (send_status != len) {
    buffer_length_ = len - send_status;
    std::memcpy(buffer_, header_.c_str() + send_status, buffer_length_);
  }
  header_ = header_.substr(len);
  if (header_.size() == 0) {
    logDebug("Header sent");
    header_sent_ = true;
  }
  return 0;
}

int HttpResponse::SendBody(int fd) {
  if (client_.status_ == "500" && !client_.file_.is_open()) {
    body_sent_ = true;
    buffer_length_ = 36;
    std::memcpy(buffer_, "<h1>500 Internal Server Error</h1>\r\n", buffer_length_);
    body_sent_ = true;
    return SendBuffer(fd);
  }
  if (getOneChunk(client_.file_) == -1)
    return -1;
 return SendBuffer(fd);
}

int HttpResponse::getOneChunk(std::fstream& file) {
  file.read(buffer_, 8000);
  if (file.fail() && !file.eof()) {
    return -1;
  }
  buffer_length_ = file.gcount();
  std::string test(buffer_, buffer_length_);
  if (buffer_length_ == 0 && file.eof()) {
    buffer_length_ = 5;
    std::memcpy(buffer_, "0\r\n\r\n", buffer_length_);
    body_sent_ = true;
    return 0;
  }
  std::ostringstream chunk_stream;
  chunk_stream << std::hex << buffer_length_ << "\r\n"; 
  chunk_stream.write(buffer_, buffer_length_);
  chunk_stream << "\r\n";
  std::string chunk_string = chunk_stream.str();
  buffer_length_ = chunk_string.size();
  std::memcpy(buffer_, chunk_string.c_str(), buffer_length_);
  return 0;
}

int HttpResponse::SendToClient(const int client_socket, const char* msg, int length) {

  int bytes_sent;
  bytes_sent = send(client_socket, msg, length, MSG_NOSIGNAL);
  if (bytes_sent == -1) {
    logInfo("send returned -1 for client socket: ", std::to_string(client_socket));
    return -1;
  }
  return bytes_sent;
}

void HttpResponse::ResetResponse() {
  header_.clear();
  status_message_.clear();
  header_sent_ = false;
  body_sent_ = false;
}

const std::map<std::string, std::string>& HttpResponse::getContTypeMap() {
  static const std::map<std::string, std::string> cont_type_map  = 
  {
    {".mp3", "audio/mpeg"},
    {".wma", "audio/x-ms-wma"},
    {".wav", "audio/x-wav"},

    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"},
    {".tiff", "image/tiff"},
    {".ico", "image/x-icon"},
    {".djvu", "image/vnd.djvu"},
    {".svg", "image/svg+xml"},

    {".css", "text/css"},
    {".csv", "text/csv"},
    {".html", "text/html"},
    {".txt", "text/plain"},

    {".mp4", "video/mp4"},
    {".mov", "video/quicktime"},
    {".avi", "video/x-msvideo"},
    {".wmv", "video/x-ms-wmv"},
    {".flv", "video/x-flv"},
    {".webm", "video/webm"},

    {".pdf", "application/pdf"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".zip", "application/zip"},
    {".js", "application/javascript"},
    {".odt", "application/vnd.oasis.opendocument.text"},
    {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {".odp", "application/vnd.oasis.opendocument.presentation"},
    {".odg", "application/vnd.oasis.opendocument.graphics"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".xul", "application/vnd.mozilla.xul+xml"}
  };
  return cont_type_map;
}

const std::map<std::string, std::string>& HttpResponse::getStatusMap() {
  static const std::map<std::string, std::string> status_map  = 
  {
    {"200", "200 OK"},
    {"300", "300 Multiple Choices"},
    {"301", "301 Moved Permanently"},
    {"302", "302 Found"},
    {"303", "303 See Other"},
    {"304", "304 Not Modified"},
    {"307", "307 Temporary Redirect"},
    {"308", "308 Permanent Redirect"},
    {"400", "400 Bad Request"},
    {"403", "403 Forbidden"},
    {"404", "404 Not Found"},
    {"405", "405 Method Not Allowed"},
    {"411", "411 Length Required"},
    {"413", "413 Content Too Large"},
    {"415", "415 Unsupported Media Type"},
    {"431", "431 Request Header Fields Too Large"},
    {"500", "500 Internal Server Error"},
    {"501", "501 Not Implemented"},
    {"502", "502 Bad Gateway"},
    {"504", "504 Gateway Timeout"},
    {"505", "505 HTTP Version Not Supported"},
  };
  return status_map;
}
