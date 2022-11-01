#include "server.h"
using namespace std;

HttpRequest::HttpRequest() {}

void HttpRequest::string2header(std::string input)
{
    istringstream iss(input);
    std::string state;
    vector<std::string> state_list;
    while(getline(iss, state))
    {
        state_list.push_back(state);
    }

    std::string token;
    vector<std::string> string_list;
    while(getline(iss, token, '\r'))
    {
        string_list.push_back(token);
        istringstream iss1(token);
        std::string token1;
        vector<std::string> string_list1;
        getline(iss1, token1, ':');
        string_list1.push_back(token1);
        string_list1[1].erase(string_list1[1].begin());
        request_header[string_list1[0]] = string_list1[1];
    }
}

HttpResponse::HttpResponse() {}

void HttpResponse::header2string()
{
    ostringstream oss;
    oss << ver << " " << code << " " << state << CRLF;
    for(auto it = response_header.begin(); it != response_header.end(); it++)
    {
        oss << it->first << ": " << it->second << CRLF;
    }
    oss << CRLF;
    oss << body << CRLF;
    send_message = oss.str();
}


Server::Server(int server_mode)
{
    mode = server_mode;
    STATE_CODE_MAP[200] = "OK";
    STATE_CODE_MAP[301] = "Moved Permanently";
    STATE_CODE_MAP[206] = "Partial Content";
    STATE_CODE_MAP[403] = "Forbidden";
    STATE_CODE_MAP[404] = "Not Found";
}


int Server::set_socket()
{
    s = socket(PF_INET, SOCK_STREAM, 0);
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

void Server::set_response_var(std::string type, std::string name)
{
    if(type == "html")
    {
        response.response_header["Content-Type"] = "text/html";
        ifstream fd;
        std::string full_path = string(ROOT_PATH)+"name";
        fd.open(full_path, ios::in);
        fd.seekg(0, std::ifstream::end);
        int file_size =fd.tellg();
        response.response_header["Content-Length"] = to_string(file_size);
        response.response_header["Connection"] = "keep-alive";
        char tmp = -1;
        while ((tmp = fd.get())!=EOF)
        {
            string tmp_s;
            tmp_s.push_back(tmp);
            response.body += tmp_s;
        }
        fd.close();
    }
    else if(type == "mp4")
    {
        std::string full_path = string(ROOT_PATH) + "name";
        int video_fd = open(full_path.c_str(), O_RDONLY);
        int video_length = lseek(video_fd, 0, SEEK_END) - lseek(video_fd, 0, SEEK_SET);
        response.response_header["Content-Type"] = "video/mpeg";
        response.response_header["Accept-Ranges"] = "bytes";
        response.response_header["Connection"] = "keep-alive";
        response.response_header["Transfer-Encoding"] = "chunked";
        response.response_header["Content-Length"] = std::to_string(video_length);

        if(response.code == 200)
        {
            char video_buf[BUF_SIZE];
            while(read(video_fd, video_buf, sizeof(video_buf)))
            {
                response.body += video_buf;
            }
        }
        else
        {

        }
    }
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
    request.string2header(buf);

    response.ver = request.ver;
    if(!access((ROOT_PATH + request.url).c_str(), 6))
    {
        cout << "Not Found" << endl;
        response.code = 404;
    }
    else if(!access((ROOT_PATH + request.url).c_str(), 0) | !access((ROOT_PATH + request.url).c_str(), 2) |
            !access((ROOT_PATH + request.url).c_str(), 4))
    {
        cout << "Forbidden" << endl;
        response.code = 403;
    }
//    else if(mode == HTTP_MODE) // turn to https, not complemented yet
//    {
//
//    }

    else
    {
        int dot_pos = request.url.find(".");
        if(dot_pos == string::npos)
        {
            cout << "Not Found" << endl;
            response.code = 404;
        }
        else
        {
            std::string file_type = request.url.substr(dot_pos, request.url.length()-dot_pos);
            cout << file_type << "file will transmit to client" << endl;

            if(file_type == "mp4" && request.request_header.find("Range") != response.response_header.end())
            {
                cout << "Partial Content" << endl;
                response.code = 206;
            }
            else
            {
                cout << "OK" << endl;
                response.code = 200;
            }

            set_response_var(file_type, request.url);
        }
    }
    response.state = STATE_CODE_MAP[response.code];
    response.header2string();
    send(client_fd, response.send_message.c_str(), response.send_message.length(), MSG_WAITALL);
    //cout << resp.response << endl;
    //send(client_fd, resp.response.c_str(), resp.response.length(), MSG_WAITALL);
    close(client_fd);
}

Server::~Server() = default;

//void Server::pack_response() // pack header and information, then send to client
//{
//    response.send_message = "";
//    response.send_message += "HTTP/1.1 200 OK\r\n";
////    response.send_message += "Content-Type:text/html\r\n\r\n";
//
//    response.send_message += "Content-Type:video/mpeg\r\n";
//    //response.send_message += "<html><head>Hello World</head></html>\r\n";
//
//    //for video
//    int video_fd = open("1.mp4", O_RDONLY);
//    int video_length = lseek(video_fd, 0, SEEK_END)- lseek(video_fd, 0, SEEK_SET);
////    response += "Content-Length:";
////    response += to_string(video_length);
//    response.send_message += "\r\n";
//    cout << response.send_message << endl;
//    char video_buf[BUF_SIZE];
//    if (read(video_fd, video_buf, sizeof(video_buf)))
//    {
//        response.send_message += video_buf;
//    }
//    response.send_message += "\r\n";
////    // for text
////    ifstream fd;
////    fd.open("index.html", ios::in);
////    char tmp = 0;
////    while ((tmp = fd.get())!=EOF)
////    {
////        string tmp_s;
////        tmp_s.push_back(tmp);
////        response.send_message += tmp_s;
////    }
////    response.send_message += "\r\n";
////    cout << "r" << response.send_message << endl;
//
//}


