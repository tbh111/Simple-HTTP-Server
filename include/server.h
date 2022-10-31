#ifndef __SERVER_H_
#define __SERVER_H_

#include <sys/socket.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <iostream>

#define HTTP_MODE 0
#define HTTPS_MODE 1
#define BUF_SIZE 4096

class HttpRequest
{
public:
	std::string method;
	std::string url;
	std::string ver;
	std::unordered_map<std::string, std::string> header;
};

class HttpResponse
{
public:
    HttpResponse();

	std::string ver;
	int code;
	std::string state;
	std::unordered_map<std::string, std::string> header;
    std::string response;
	char *body;

    void pack_response();
};

class Server
{
public:
    Server(int server_mode);
	~Server();
	int set_socket();
	void accept_request();
	
private:
    HttpResponse response;
    HttpRequest request;

    struct sockaddr_in server_addr;

    void encode_response();
    void decode_request();

	int http_port = 80;
    int https_port = 443;
    int mode = HTTP_MODE;
    int s = -1;
    char buf[BUF_SIZE] = {0};
    std::string encoded_response;
};

#endif