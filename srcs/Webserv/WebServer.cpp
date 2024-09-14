
#include "WebServer.hpp"
#include "HttpResponse.hpp"
#include "HttpParser.hpp"
// #include <sstream>
// #include <fstream>
// #include <string>
// #include <iterator>
// #include <optional>


extern bool showResponse;
extern bool showRequest;


WebServer::WebServer(const char *m_ipAddress, const char *m_port)
	: TcpListener(m_ipAddress, m_port){
	;
}



void WebServer::onMessageRecieved(const int clientSocket, const char *msg, 
  int bytesIn, short revents){
	
  (void)bytesIn;
  (void)revents;
  /* TODO if bytesIn == MAXBYTES then recv until the whole message is sent 
    see 100 Continue status message
    https://www.w3.org/Protocols/rfc2616/rfc2616-sec8.html#:~:text=Requirements%20for%20HTTP/1.1%20origin%20servers%3A*/

	//TODO: check if the header contains "Connection: close"

  // std::istringstream iss(msg);
  // std::vector<std::string> parsed
	//   ((std::istream_iterator<std::string>(iss)), (std::istream_iterator<std::string>()));
  // std::cout << parsed[1] << std::endl;
  
  HttpParser parser;
  HttpResponse response;

  if (!parser.parseRequest(msg))
    std::cout << "false on parseRequest returned" << std::endl;
  else if (showRequest)
    std::cout << parser.getrequestBody() << std::endl;

  response.assignContType(parser.getResourcePath()); 
  response.openFile(parser.getResourcePath());
  response.composeHeader();
  sendToClient(clientSocket, response.getHeader().c_str(), response.getHeader().size());
  
  response.setErrorCode(123); // temp to shut the compiler, cause parser errorCodes broken for now

  const int chunk_size = 1024;
  char buffer[chunk_size]{};
  int i = 0;
  /* TODO: check if the handling of SIGINT on send error is needed  
    It would sigint on too many failed send() attempts*/
  std::ifstream& file = response.getFile();
  if (file.is_open()){

    std::streamsize bytes_read;
    while (file) {
      file.read(buffer, chunk_size);
      bytes_read = file.gcount(); 
      if (bytes_read == -1)
      {
        std::cout << "bytes_read returned -1" << std::endl;
        sendToClient(clientSocket, "0\r\n\r\n", 5);
        break;
      }
      // Send the size of the chunk in hexadecimal
      std::ostringstream chunk_size_hex;
      chunk_size_hex << std::hex << bytes_read << "\r\n";
      if (sendToClient(clientSocket, chunk_size_hex.str().c_str(), chunk_size_hex.str().length()) == -1 || 
        sendToClient(clientSocket, buffer, bytes_read) == -1 ||
        sendToClient(clientSocket, "\r\n", 2) == -1){
          perror("send :");
          break ;
      }
      i ++;
    }
    std::cout << "sent a chunk " << i << " times and the last one was " << bytes_read << std::endl;
    // Send the final zero-length chunk to signal the end
    if (sendToClient(clientSocket, "0\r\n\r\n", 5) == -1)
      perror("send 4:");
    else
      std::cout << "sent the closing chunk" << std::endl;
  }
  else
    if (sendToClient(clientSocket, "<h1>404 Not Found</h1>", 23) == -1)
      perror("send 5:");
  file.close(); 
}

void WebServer::onClientConnected()
{
	;
}

void WebServer::onClientDisconected()
{
	;
}

// void WebServer::sendVideo(int clientSocket, const char *msg, int length)
// {
// 	// if (parsed.size() >= 3 && parsed[0] == "GET")
// 	// {	
// 		const char *videoPath = "./pages/video.mp4";  // Path to the video file
// 		/* binary: for non text files, skips newlines
// 			ate: poistion the pointer At the End of file upon opening for determening the size*/
//     	std::ifstream videoFile(videoPath, std::ios::binary | std::ios::ate);
// 		if (!videoFile.is_open())
// 			std::cerr << "Failed to open file\n";
// 		/* tellg returns position of file pointer, gives the size of the file in bytes
// 			since it was at the end of file */
// 		std::streampos fileSize = videoFile.tellg();
// 		videoFile.seekg(0, std::ios::beg);
// 	// }

// 	oss << "Content-Type: video/mp4\r\n";
	// char fileBuffer[4096];
// 	oss << "Content-Length: " << std::to_string(fileSize)  << "\r\n";
	// while (videoFile.read(fileBuffer, sizeof(fileBuffer))) {
    //     send(clientSocket, fileBuffer, sizeof(fileBuffer), 0);
	// 	std::cout << "here22" << std::endl;
    // }
    // if (videoFile.gcount() > 0) {
    //     send(clientSocket, fileBuffer, videoFile.gcount(), 0);
    // }
	//  videoFile.close();
// }

