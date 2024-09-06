
#include "WebServer.hpp"
#include <sstream>
#include <fstream>
#include <string>



WebServer::WebServer()
{
	std::cout << "Default Constructor for WebServer called" << std::endl;
}

WebServer::WebServer(const char *m_ipAddress, const char *m_port)
	: TcpListener(m_ipAddress, m_port)
{
	std::cout << "Paramterized Constructor for WebServer called" << std::endl;
}

WebServer::WebServer(const WebServer &other)
{
	std::cout << "Copy constuctor for WebServer called" << std::endl;
	*this = other;
}

WebServer::~WebServer()
{
	std::cout << "Destructor for WebServer called" << std::endl;
}

WebServer& WebServer::operator=(const WebServer& other)
{
	std::cout << "Assignment operator for WebServer called" << std::endl;
	if (this != &other)
	{
		return *this;
	}
	return *this;
}

void WebServer::onMessageRecieved(int clientSocket, const char *msg, int length)
{
	//GET /index.html HTTP/1.1

	//Parse out the document requested
	//Open the document ini the loacl files system
	//Write the document back to the client
	(void)length;
	(void)msg;

	std::ifstream f("./pages/index.html");
	if (!f.is_open())
		std::cerr << "Failed to open index.html\n";
	std::string content = "<h1>404 Not Found</h1>";
	int errorCode = 404;

	if (f.good())
	{
		while (std::getline(f, content, '\0'))
			;
		errorCode = 200;
	}

	f.close(); 

	std::ostringstream oss;
	oss << "HTTP/1.1 " << errorCode << " OK\r\n";
	oss << "Cache-Control: no-cache, private\r\n";
	oss << "Content-Type: text/html\r\n";
	oss << "Content-Length: " << content.size()  << "\r\n";
	oss << "\r\n";
	oss << content;

	std::string output = oss.str();
	int size = output.size() + 1;

	if(sendToClient(clientSocket, oss.str().c_str(), size) < 0)
		exit (123);

}

void WebServer::onClientConnected()
{
	;
}

void WebServer::onClientDisconected()
{
	;
}
