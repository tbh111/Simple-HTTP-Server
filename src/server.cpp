#include "server.h"
using namespace std;

HttpRequest::HttpRequest() {}

void HttpRequest::string2header(std::string input)
{
    istringstream iss(input);
    std::string state;
    vector<std::string> state_list;
    getline(iss, state);
    string_split(state, ' ', state_list);
    if(state_list.size() != 3)
    {
        cout << "state error" << endl;
    }
    method = state_list[0];
    url = state_list[1];
    ver = state_list[2];
    cout << method << " " << url << " " << ver << endl;;

    std::string token;
    vector<std::string> token_list;
    while(getline(iss, token))
    {
        if(token.length() > 1)
        {
            string_split_first(token, ':', token_list);
            token_list[1].erase(token_list[1].begin());
            request_header[token_list[0]] = token_list[1];
            cout << "req header: " << token_list[0] << ": " << token_list[1] << endl;
            token_list.clear();
            token.clear();
        }
    }
}

HttpResponse::HttpResponse() {}

std::string HttpResponse::header2string(int file_type)
{
    std::string blank = " ";
    std::string state_string = ver_res;// + blank + to_string(code) + blank + state + CRLF;
    state_string += blank;
    state_string += to_string(code);
    state_string += blank;
    state_string += state;
    state_string += CRLF;
    cout << ver_res << " " << to_string(code) << " " << state << endl;
    cout << state_string << endl;
    std::ostringstream oss;
    oss.str("");

    for(auto & it : response_header)
    {
        oss << it.first << ": " << it.second << CRLF;
    }
    oss << CRLF;
    std::string oss_string = oss.str();
    return state_string + oss_string;
//
//    message_length =  header_length + body_length;
//    char* message = new char[message_length + 4];
//    if(file_type == HTML)
//    {
//        oss << body << CRLF;
//        oss_string = oss.str();
//        memcpy(message, (char*)(state_string + oss_string).data(), message_length);
//    }
//    else if(file_type == MP4)
//    {
//        memcpy(&(message[0]), (state_string + oss_string).data(), header_length);
//        int count = 0;
//        for(auto & i : send_queue)
//        {
//            count++;
//            if(count == send_queue.size())
//            {
//                memcpy(&(message[header_length + video_buf_length*(count-1)+3]), i, video_buf_length+3);
//            }
//            else
//            {
//                memcpy(&(message[header_length + video_buf_length*(count-1)]), i, video_buf_length);
//            }
//        }
//    }
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
    s = socket(AF_INET, SOCK_STREAM, 0);
    int timeout = 10;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &timeout, sizeof(timeout));
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

void Server::set_response_var(std::string type)
{
    if(type == "html")
    {
        send_type = HTML;
        response->response_header["Content-Type"] = "text/html";
        std::ifstream fd;
        std::string full_path = ROOT_PATH + request->url;
        fd.open(full_path, ios::in);
        fd.seekg(0, std::ifstream::end);
        int file_size =fd.tellg();
        fd.seekg(0, std::ifstream::beg);
        response->response_header["Content-Length"] = to_string(file_size);
        response->response_header["Connection"] = "keep-alive";
        response->body_length = file_size;
        char tmp = 0;
        while ((tmp = fd.get())!=EOF)
        {
            cout << tmp;
            std::string tmp_s;
            tmp_s.push_back(tmp);
            //response->body += tmp_s;
        }
        fd.close();
    }
    else if(type == "mp4")
    {
        send_type = MP4;
        std::string full_path = std::string(ROOT_PATH) + request->url; // path in server
        cout << full_path << endl;

        FILE* video_fp;
        if((video_fp = fopen(full_path.c_str(), "rb")) == NULL)
        {
            cout << "open error" << endl;
            exit(-1);
        }
        fseek(video_fp, 0, SEEK_END);
        int video_length = ftell(video_fp);

        // int video_fd = open(full_path.c_str(), ios::in|ios::binary); // open video in read-only mode
        response->response_header["Content-Type"] = "video/mpeg";
        response->response_header["Accept-Ranges"] = "bytes";
        response->response_header["Connection"] = "keep-alive";
        //response->response_header["Transfer-Encoding"] = "chunked";
        response->response_header["Content-Length"] = std::to_string(video_length);

        if(response->code == 200)
        {
            //char video_buf[BUF_SIZE];
            while(true)
            {
                cout << "received 200 code" << endl;
                break;
            }
//            while(read(video_fd, video_buf, sizeof(video_buf)))
//            {
//                response->body += video_buf;
//            }
        }
        else
        {
            int range_length = request->request_header["Range"].length();
            std::string request_range = request->request_header["Range"].substr(6, range_length-7);
            int hyphen_pos = request_range.find("-");
            std::string start_string = request_range.substr(0, hyphen_pos);
            std::string end_string = "";
            cout << "hyphen pos " << hyphen_pos << " " << request_range.length() << endl;
            if(hyphen_pos != request_range.length()-1)
            {
                end_string = request_range.substr(hyphen_pos+1);
            }
            long start_addr = -1, end_addr = -1;
            cout << "range " << request_range << endl;
            cout << start_string.empty() << " " << end_string.empty() << endl;
            if(!start_string.empty() && end_string.empty()) // x-
            {
                start_addr = stol(start_string);
                end_addr = video_length - 1;
            }
            else if(!start_string.empty() && !end_string.empty()) // x-x
            {
                start_addr = stol(start_string);
                end_addr = stoi(end_string);
            }
            else if(start_string.empty() && !end_string.empty()) // -x
            {
                start_addr = video_length - stol(end_string);
                end_addr = video_length - 1;
            }
            else
            {
                cout << "range error" << endl;
                return;
            }
            long read_length = end_addr - start_addr + 1;
            response->body_length = read_length+3;

            ostringstream content_range;
            content_range << "bytes " << start_addr << "-" << end_addr << "/" << video_length;
            response->response_header["Content-Length"] = to_string(read_length);
            response->response_header["Content-Range"] = content_range.str();
            cout << "reading: " << start_addr << " to " << end_addr << endl;
            int send_count = read_length / video_buf_length + 1;

            while (true)
            {
                fseek(video_fp, start_addr, SEEK_SET);
                char video_buf[video_buf_length];
                fread(video_buf, 1, video_buf_length, video_fp);
                memcpy(&(response->video_body[0]), video_buf, video_buf_length);
                char* tmp_body = new char[video_buf_length]; // memory leak!!!
                memcpy(&(tmp_body[0]), &(response->video_body[0]), video_buf_length);
                response->send_queue.push_back(&(tmp_body[0]));
                start_addr += video_buf_length;

                if(start_addr >= end_addr)
                {
                    fread(video_buf, 1, video_buf_length, video_fp);
                    memcpy(&(response->video_body_EOF[0]), video_buf, video_buf_length);
                    memset(&(response->video_body_EOF[video_buf_length]), '\r', 1);
                    memset(&(response->video_body_EOF[video_buf_length+1]), '\n', 1);
                    memset(&(response->video_body_EOF[video_buf_length+2]), '\0', 1);
                    char* tmp_body_EOF = new char[video_buf_length+3];

                    memcpy(&(tmp_body_EOF[0]), &(response->video_body_EOF[0]), video_buf_length+3);
                    response->send_queue.push_back(&(tmp_body_EOF[0]));
                    fclose(video_fp);
                    break;
                }
            }
            fseek(video_fp, 0, SEEK_END);

//            while(read_length > video_buf_length)
//            {
//                end_addr = start_addr + video_buf_length;
//                content_range << "bytes " << start_addr << "-" << end_addr << "/" << video_length;
//                response->response_header["Content-Length"] = to_string(video_buf_length);
//                cout << "reading: " << start_addr << " to " << end_addr << endl;
//                response->body_length = video_buf_length+3;
//
//                fseek(video_fp, start_addr, SEEK_SET);
//                char video_buf[video_buf_length];
//                fread(video_buf, 1, video_buf_length, video_fp);
//                memcpy(&(response->video_body[0]), video_buf, video_buf_length);
//                memcpy(&(response->body[0]), &(response->video_body[0]), video_buf_length+3);
//
//
//                start_addr += 4096;
//                read_length -= 4096;
//            }
//            end_addr = video_length - 1;
//            content_range << "bytes " << start_addr << "-" << end_addr << "/" << video_length;
//            response->response_header["Content-Length"] = to_string(video_buf_length);
//            cout << "reading: " << start_addr << " to " << end_addr << endl;
//            response->body_length = video_buf_length+3;
//
//            if(read_length > video_buf_length)
//            {
//                end_addr = start_addr + video_buf_length;
//                content_range << "bytes " << start_addr << "-" << end_addr << "/" << video_length;
//                response->response_header["Content-Length"] = to_string(video_buf_length);
//                cout << "reading: " << start_addr << " to " << end_addr << endl;
//                response->body_length = video_buf_length+3;
//            }
//            else
//            {
//                content_range << "bytes " << start_addr << "-" << end_addr << "/" << video_length;
//                response->response_header["Content-Length"] = std::to_string(read_length);
//                cout << "reading: " << start_addr << " to " << end_addr << endl;
//                response->body_length = read_length+3;
//            }
//            response->response_header["Content-Range"] = content_range.str();
//
//            fseek(video_fp, start_addr, SEEK_SET);
//            char video_buf[video_buf_length];
//            fread(video_buf, 1, video_buf_length, video_fp);
//            memcpy(&(response->video_body[0]), video_buf, video_buf_length);
//            memset(&(response->video_body[video_buf_length]), '\r', 1);
//            memset(&(response->video_body[video_buf_length+1]), '\n', 1);
//            memset(&(response->video_body[video_buf_length+2]), '\0', 1);
//            memcpy(&(response->body[0]), &(response->video_body[0]), video_buf_length+3);

            // read video
//            lseek(video_fd, start_addr, SEEK_SET);
//            char* video_buf = new char[1000];
//            int test = read(video_fd, video_buf, 999);
//            video_buf[999] = '\0';
//            cout << "buf:" << test << endl;
//            std::string video_string = video_buf;
//            response->body = video_string;
//            fseek(video_fp, start_addr, SEEK_SET);
//            char video_buf[video_buf_length];
//            fread(video_buf, 1, video_buf_length, video_fp);
//            memcpy(&(response->video_body[0]), video_buf, video_buf_length);
//            memset(&(response->video_body[video_buf_length]), '\r', 1);
//            memset(&(response->video_body[video_buf_length+1]), '\n', 1);
//            memset(&(response->video_body[video_buf_length+2]), '\0', 1);
//            memcpy(&(response->body[0]), &(response->video_body[0]), video_buf_length+3);
        }
        //fclose(video_fp);
    }
}

void Server::accept_request()
{
    response = new HttpResponse;
    request = new HttpRequest;
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
    memset(buf, 0, BUF_SIZE);
    int size = read(client_fd, buf, sizeof(buf));
    request->string2header(buf);
    response->ver_res = request->ver;
    cout << "url " << request->url << endl;
    cout << "path " << ROOT_PATH + request->url << endl;
    cout << "access " << access((ROOT_PATH + request->url).c_str(), F_OK) << endl;

    if(access((ROOT_PATH + request->url).c_str(), F_OK) < 0)
    {
        cout << "Not Found" << endl;
        response->code = 404;
    }
    else if(access((ROOT_PATH + request->url).c_str(), R_OK) < 0)
    {
        cout << "Forbidden" << endl;
        response->code = 403;
    }
//    else if(mode == HTTP_MODE) // turn to https, not complemented yet
//    {
//
//    }

    else
    {
        int dot_pos = request->url.find(".");
        if(dot_pos == string::npos)
        {
            cout << "Not Found" << endl;
            response->code = 404;
        }
        else
        {
            std::string file_type = request->url.substr(dot_pos+1, request->url.length()-dot_pos);
            cout << file_type << " file will transmit to client" << endl;
            if(file_type == "mp4" && request->request_header.find("Range") != response->response_header.end())
            {
                cout << "Partial Content" << endl;
                response->code = 206;
            }
            else
            {
                cout << "OK" << endl;
                response->code = 200;
            }

            set_response_var(file_type);
        }
    }
    response->state = STATE_CODE_MAP[response->code];
    response->ver_res = "HTTP/1.1";
    std::string message = response->header2string(send_type);
    send(client_fd, message.c_str(), message.length(), MSG_WAITALL);
    int count = 0;
    ulong data_length = 0;
    for(auto it=response->send_queue.begin(); it!=response->send_queue.end(); it++)
    {
        if(count++ == response->send_queue.size())
        {
            data_length = (response->body_length-3) % video_buf_length + 3;
        }
        else
        {
            data_length = video_buf_length;
        }
        char* data = new char[data_length];
        memcpy(data, (*it), data_length);
        send(client_fd, data, data_length, MSG_WAITALL);
        delete [] data;
    }


    //close(client_fd);

    delete request;
    delete response;
}

Server::~Server() = default;

void string_split(const std::string& str, const char split, std::vector<std::string>& res)
{
    if(str.empty())
        return;
    std::string str_fix = str + split;
    size_t pos = str_fix.find(split);
    while(pos != str_fix.npos)
    {
        std::string tmp = str_fix.substr(0, pos);
        res.push_back(tmp);
        str_fix = str_fix.substr(pos+1, str_fix.size());
        pos = str_fix.find(split);
    }
}

void string_split_first(const std::string& str1, const char split1, std::vector<std::string>& res1)
{
    if(str1.empty())
        return;
    size_t pos = str1.find(split1);
    if(pos != str1.npos)
    {
        std::string tmp = str1.substr(0, pos);
        std::string tmp1 = str1.substr(pos+1, str1.size());
        res1.push_back(tmp);
        res1.push_back(tmp1);
    }
}
//void Server::pack_response() // pack header and information, then send to client
//{
//    response->send_message = "";
//    response->send_message += "HTTP/1.1 200 OK\r\n";
////    response->send_message += "Content-Type:text/html\r\n\r\n";
//
//    response->send_message += "Content-Type:video/mpeg\r\n";
//    //response->send_message += "<html><head>Hello World</head></html>\r\n";
//
//    //for video
//    int video_fd = open("1.mp4", O_RDONLY);
//    int video_length = lseek(video_fd, 0, SEEK_END)- lseek(video_fd, 0, SEEK_SET);
////    response += "Content-Length:";
////    response += to_string(video_length);
//    response->send_message += "\r\n";
//    cout << response->send_message << endl;
//    char video_buf[BUF_SIZE];
//    if (read(video_fd, video_buf, sizeof(video_buf)))
//    {
//        response->send_message += video_buf;
//    }
//    response->send_message += "\r\n";
////    // for text
////    ifstream fd;
////    fd.open("index.html", ios::in);
////    char tmp = 0;
////    while ((tmp = fd.get())!=EOF)
////    {
////        string tmp_s;
////        tmp_s.push_back(tmp);
////        response->send_message += tmp_s;
////    }
////    response->send_message += "\r\n";
////    cout << "r" << response->send_message << endl;
//
//}


