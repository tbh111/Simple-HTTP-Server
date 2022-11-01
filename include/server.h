#ifndef __SERVER_H_
#define __SERVER_H_

#include <sys/socket.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#define HTTP_MODE 0
#define HTTPS_MODE 1
#define BUF_SIZE 4096
#define CRLF "\r\n"
#define ROOT_PATH "/home/mininet/Simple-HTTP-Server"

class HttpRequest
{
public:
    HttpRequest();
	std::string method;
	std::string url;
	std::string ver;
	std::unordered_map<std::string, std::string> request_header;
    void string2header(std::string input);
};

class HttpResponse
{
public:
    HttpResponse();
	std::string ver;
	int code;
	std::string state;
    std::string send_message = "";
    std::string body;
	std::unordered_map<std::string, std::string> response_header;

    void header2string();
};

class Server
{
public:
    Server(int server_mode);
	~Server();
	int set_socket();
	void accept_request();
    std::map<int, std::string> STATE_CODE_MAP;

private:
    HttpResponse response;
    HttpRequest request;

    struct sockaddr_in server_addr;
    void set_response_var(std::string type, std::string name);
    // void pack_response()

	int http_port = 80;
    int https_port = 443;
    int mode = HTTP_MODE;
    int s = -1;
    char buf[BUF_SIZE] = {0};
    std::string encoded_response;
};

#endif