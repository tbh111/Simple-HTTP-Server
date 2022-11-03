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
#define HTML 0
#define MP4 1
#define BUF_SIZE 4096
#define CRLF "\r\n"
#define ROOT_PATH "/home/mininet/Simple-HTTP-Server/test"
const long video_buf_length = 4096;//4096;
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
	std::string ver_res;
	int code;
	std::string state;

    char body[video_buf_length];
    char video_body[video_buf_length];
    char body_EOF[video_buf_length+3];
    char video_body_EOF[video_buf_length+3];
    std::vector<char*> send_queue;

    u_long body_length = 0;
	std::unordered_map<std::string, std::string> response_header;

    std::string header2string(int file_type);
};

class Server
{
public:
    Server(int server_mode);
	~Server();
	int set_socket();
    int stop_flag = 0;
	void accept_request();
    std::map<int, std::string> STATE_CODE_MAP;
    int send_type = HTML;
private:
    HttpResponse* response;
    HttpRequest* request;

    struct sockaddr_in server_addr;
    void set_response_var(std::string type);

    // void pack_response()

	int http_port = 80;
    int https_port = 443;
    int mode = HTTP_MODE;
    int s = -1;
    char buf[BUF_SIZE];

};
void string_split(const std::string& str, const char split, std::vector<std::string>& res);
void string_split_first(const std::string& str1, const char split1, std::vector<std::string>& res1);
#endif