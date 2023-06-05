#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <time.h>
using namespace std;

enum class HTTPRequest {
	TRACE,
	DELETER,
	PUT,
	POST,
	HEAD,
	GET,
	OPTIONS
};

struct SocketState
{
	SOCKET id;					// Socket handle
	int	recv;					// Receiving?
	int	send;					// Sending?
	HTTPRequest sendSubType;	// Sending sub-type
	char buffer[128];
	int len;
};

const char* FILE_PATH = "C:\\Temp\\html_files\\";
const int TCP_PORT = 27015;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;
const int BUFF_SIZE = 255;

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;
string method, buffer, queryString;

bool addSocket(SOCKET id, int what);
void acceptConnection(int index, SocketState* sockets);
void receiveMessage(int index, SocketState* sockets);
void removeSocket(int index, SocketState* sockets);
void sendMessage(int index, SocketState* sockets);
int putRequest(struct SocketState* socket);
string getRequest(int index, SocketState* sockets, bool isGet);
HTTPRequest getRequestNumber(string recvBuff);
string handlePutRequest(int index, SocketState* sockets);
string handlePostRequest(int index, SocketState* sockets);
string handleGetRequest(int index, SocketState* sockets, bool isGet);


void main()
{
	// Initialize Winsock (Windows Sockets).

	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData;

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "TCP Server: Error at WSAStartup()\n";
		return;
	}

	// Server side:
	// Create and bind a socket to an internet address.àæ 
	// Listen through the socket for incoming connections.

	// After initialization, a SOCKET object is ready to be instantiated.

	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "TCP Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// For a server to communicate on a network, it must bind the socket to 
	// a network address.

	// Need to assemble the required data for connection in sockaddr structure.

	// Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(TCP_PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService)))
	{
		cout << "TCP Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "TCP Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN);

	// Accept connections and handles them one by one.
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//

		// Close connection if a socket is not activated for more then 2 minutes
		struct timeval timeout;
		timeout.tv_sec = 120;
		timeout.tv_usec = 0;

		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, &timeout);

		if (nfd == SOCKET_ERROR)
		{
			cout << "TCP Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i, sockets);
					break;

				case RECEIVE:
					receiveMessage(i, sockets);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				sendMessage(i, sockets);
			}
		}
		//cout << "TCP Server: nfd = " << nfd << endl;
	}

	// Closing connections and Winsock.
	cout << "TCP Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}


bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index, SocketState* sockets)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index, SocketState* sockets)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "TCP Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "TCP Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	// Set the socket to be in non-blocking mode.
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "TCP Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index, SocketState* sockets)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "TCP Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index, sockets);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index, sockets);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "TCP Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" message.\n";

		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			// Get buffer from socket and get from it the method and queryString
			buffer = sockets[index].buffer;
			cout << "Buffer: " << buffer << endl;

			method = buffer.substr(0, buffer.find(' '));
			buffer = buffer.substr(buffer.find(' ') + 2, string::npos);
			queryString = buffer.substr(0, buffer.find(' '));

			//update request in buffer
			sockets[index].send = SEND;
			sockets[index].recv = EMPTY;
			sockets[index].sendSubType = getRequestNumber(method);
			cout << "Buffer after substring: " << buffer << endl;
			cout << "Method: " << method << endl;
			cout << "queryString: " << queryString << endl;
		}
	}
}

void sendMessage(int index, SocketState* sockets)
{
	int bytesSent = 0;
	char sendBuff[BUFF_SIZE];

	string response, fileAddress = FILE_PATH;

	SOCKET msgSocket = sockets[index].id;

	// Get the current time
	time_t timer;
	time(&timer);

	switch (sockets[index].sendSubType)
	{
	case (HTTPRequest::TRACE):
		response = "HTTP/1.1 200 OK";
		response += "\r\nDate: ";
		response += ctime(&timer);
		response += "\r\nContent-Type: html";
		response += "\r\nContent-length: ";
		response += to_string(response.size() + strlen("\r\nRequest: TRACE\r\n") + buffer.size());
		response += "\r\nRequest: TRACE\r\n";
		response += buffer;
		response += "\r\n\r\n";
		break;
	case (HTTPRequest::DELETER):		
		fileAddress += queryString; // Adds the file name
		if (remove(fileAddress.c_str()) == 0)
			response = "HTTP/1.1 204 No Content"; // The server successfully processed the request, but is not returning any content 
		else
			response = "HTTP/1.1 404 Not Found"; // Failed to remove resource

		response += "\r\nContent-length: ";
		response += to_string(response.size() + strlen("\r\nRequest: DELETE\r\n\r\n"));
		response += "\r\nRequest: DELETE\r\n\r\n";
		break;
	case (HTTPRequest::PUT):
		response = handlePutRequest(index, sockets);
		response += "\r\nContent-length: ";
		response += to_string(response.size() + strlen("\nRequest: PUT\n"));
		response += "\r\nRequest: PUT\r\n\r\n";
		break;
	case (HTTPRequest::POST):
		cout << endl << "POST request has been recieved: \n" << queryString << endl;
		response = "HTTP/1.1 200 OK";
		response += "\r\nContent-message: Post message was outputed on the server's console.";
		response += "\r\nContent-length: ";
		response += to_string(response.size() + strlen("\r\nRequest: POST\r\n\r\n"));
		response += "\r\nRequest: POST\r\n\r\n";
		break;
	case (HTTPRequest::HEAD):
		response = handleGetRequest(index, sockets, false);
		response += "Content-length: ";
		response += to_string(response.size() + strlen("\r\nRequest: HEAD\r\n"));
		response += "\r\nRequest: HEAD\r\n\r\n";
		break;
	case (HTTPRequest::GET):
		response = handleGetRequest(index, sockets, true);
		response += "\nContent-length: ";
		response += to_string(response.size() + strlen("\nRequest: GET\n"));
		response += "\r\nRequest: GET\r\n\r\n";
		break;
	case (HTTPRequest::OPTIONS):
		//response = "HTTP/1.1 204 No Content\r\nAllow: OPTIONS, GET, HEAD, POST, TRACE, PUT\r\nDATE: ";
		//response += ctime(&timer);
		response = "HTTP/1.1 204 No Content\r\n Allow: OPTIONS, GET, HEAD, POST, TRACE, PUT\r\n";
		response += "Request: OPTIONS\r\n\r\n";
		break;
	default:
		response = "Request is not allowed. Ask for OPTIONS.\r\n\r\n";
		break;
	}

	strcpy(sendBuff, response.c_str());

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "TCP Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "\nTCP Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n\n";

	sockets[index].send = IDLE;
}

int putRequest(int index, SocketState* sockets) {
	string content, buffer, address;
	int statusCode = 200;
	size_t found;
	fstream oFile;

	buffer = (string)sockets[index].buffer;
	address = FILE_PATH + queryString;
	oFile.open(address);

	if (!oFile.good())
	{
		oFile.open(address, ios::out); // Created new file
		statusCode = 201;
	}
	if (!oFile.good())
	{
		cout << "HTTP Server - Failed to open file: " << WSAGetLastError() << endl;
		return 0; // Failed open file
	}

	found = buffer.find("\r\n\r\n");
	content = &buffer[found + 4];
	if (content.length() == 0)
		statusCode = 204; // No content
	else
		oFile << content;

	oFile.close();
	return statusCode;
}

string handlePutRequest(int index, SocketState* sockets) {
	switch (putRequest(index, sockets))
	{
	case 0:
	{
		return "HTTP/1.1 412 Precondition failed";
	}
	case 200:
	{
		return "HTTP/1.1 200 OK";
	}
	case 201:
	{
		return "HTTP/1.1 201 Created";
	}
	case 204:
	{
		return "HTTP/1.1 204 No Content";
	}
	default:
	{
		return "HTTP/1.1 501 Not Implemented";
	}
	}
}

HTTPRequest getRequestNumber(string recvBuff) {
	if (recvBuff == "TRACE")
		return HTTPRequest::TRACE;
	else if (recvBuff == "DELETE")
		return HTTPRequest::DELETER;
	else if (recvBuff == "POST")
		return HTTPRequest::POST;
	else if (recvBuff == "HEAD")
		return HTTPRequest::HEAD;
	else if (recvBuff == "PUT")
		return HTTPRequest::PUT;
	else if (recvBuff == "GET")
		return HTTPRequest::GET;
	else
		return HTTPRequest::OPTIONS;
}

string getRequest(int index, SocketState* sockets, bool isGet) {
	string fileName = queryString.substr(0, queryString.find('.')),
		fileSuffix = queryString.substr(6, queryString.find('?') - queryString.find('.') - 1),
		param = queryString.substr(queryString.find('?') + 1, string::npos);

	if ((fileSuffix != "html" && fileSuffix != "txt") || (param != "lang=he" && param != "lang=fr" && param != "lang=en")) {
		cout << "Error! wrong params!";
		return "400";
	}

	string lang = "english";
	if (param == "lang=he") {
		lang = "hebrew";
	}
	else if (param == "lang=fr") {
		lang = "french";
	}

	// get content from file to string 
	ifstream file;
	string address = FILE_PATH;
	address += (lang + "\\" + fileName + "." + fileSuffix);

	file.open(address);
	string content = "", line;

	if (file) {
		if (isGet) {
			while (getline(file, line))
				content += line;
		}
		else {
			content = "200";
		}
	}
	else {
		content = "404";
	}


	file.close();

	return content;
}

string handleGetRequest(int index, SocketState* sockets, bool isGet) {
	string content = getRequest(index, sockets, isGet);

	if (content == "") {
		return "HTTP/1.1 204 No Content";
	}
	else if (content == "400") {
		return "HTTP/1.1 400 Bad Request";
	}
	else if (content == "404") {
		return "HTTP/1.1 404 Not Found";
	}
	else {
		string response = "HTTP/1.1 200 OK\n";
		response += content;
		return response;
	}
}