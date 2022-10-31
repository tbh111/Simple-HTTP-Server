#include "server.h"
using namespace std;

HttpResponse::HttpResponse()
{

}

void HttpResponse::pack_response()
{
    response = "";
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type:text/html\r\n";
    response += "\r\n";
    response += "<html><head>Hello World</head></html>\r\n";
}

Server::Server(int server_mode)
{
    mode = server_mode;
}

int Server::set_socket()
{
    s = socket(AF_INET, SOCK_STREAM, 0);
    if(s == -1)
    {
        cout <<"cannot open socket" << endl;
        return -1;
    }
    cout << "socket opened: " << s << endl;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if(mode == HTTPS_MODE)
    {
        server_addr.sin_port = htons(https_port);
    }
    else
    {
        server_addr.sin_port = htons(http_port);
    }
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int res = bind(s, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(res == -1)
    {
        cout << "bind failed" << endl;
        return -1;
    }
    cout << "bind success: " << res << endl;
    int listen_res = listen(s, 10);
    if(listen_res == -1)
    {
        cout << "listen failed" <<endl;
        return -1;
    }
    cout << "listen client: " << listen_res << endl;
    return 0;
}

void Server::accept_request()
{
    struct sockaddr_in client_addr;
    int len = sizeof(client_addr);
    int client_fd = accept(s, (struct sockaddr*)&client_addr, (socklen_t*)&len); //blocked waiting
    if(client_fd == -1)
    {
        return;
    }
    // cout << client_fd << endl;
    char* ip = inet_ntoa(client_addr.sin_addr);
    cout << "connect to " << ip << endl;
    fill_n(buf, BUF_SIZE, 0);
    int size = read(client_fd, buf, sizeof(buf));
    cout << "get request: \n" << string(buf) << endl;
    HttpResponse resp;
    resp.pack_response();
    send(client_fd, resp.response.c_str(), resp.response.length(), MSG_WAITALL);
    close(client_fd);
}

Server::~Server() = default;