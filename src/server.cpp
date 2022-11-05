#include "server.h"

HttpRequest::HttpRequest() {}

void HttpRequest::string2header(std::string input) // transform request string to header map
{
    std::istringstream iss(input);
    std::string state;
    std::vector<std::string> state_list;
    getline(iss, state);
    if(state.empty())
    {
        return;
    }
    state = state.erase(state.length()-1, 1);
    string_split(state, ' ', state_list);
    if(state_list.size() != 3)
    {
        logging::ERROR("request state error");
        return;
    }
    method = state_list[0];
    url = state_list[1];
    ver = state_list[2];
    logging::DEBUG(method + " " + url + " " + ver);

    std::string token;
    std::vector<std::string> token_list;
    while(getline(iss, token))
    {
        if(token.length() > 1)
        {
            string_split_first(token, ':', token_list);
            token_list[1].erase(token_list[1].begin());
            request_header[token_list[0]] = token_list[1].erase(token_list[1].length()-1, 1);
            logging::DEBUG("req header: " + token_list[0] + ": " + token_list[1]);
            token_list.clear();
            token.clear();
        }
    }
}

HttpResponse::HttpResponse() {}

std::string HttpResponse::header2string(int file_type) // transform header map to string
{
    std::string blank = " ";
    std::string state_string = ver_res;// + blank + to_string(code) + blank + state + CRLF;
    state_string += blank;
    state_string += std::to_string(code);
    state_string += blank;
    state_string += state;
    state_string += CRLF;
    // std::cout << ver_res << " " << std::to_string(code) << " " << state << std::endl;
    // std::cout << state_string << std::endl;
    logging::DEBUG("send response: \n" + state_string);
    std::ostringstream oss;
    oss.str("");

    for(auto & it : response_header)
    {
        oss << it.first << ": " << it.second << CRLF;
    }
    oss << CRLF;
    std::string oss_string = oss.str();
    return state_string + oss_string;
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
    // init ssl

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD* method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    if(!ctx)
    {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // load certificate amd private key
    std::string cert_path = ROOT_PATH + std::string("/keys/cacert.pem");
    std::string privkey_path = ROOT_PATH + std::string("/keys/privkey.pem");
    int cert_fp = SSL_CTX_use_certificate_file(ctx, cert_path.c_str(), SSL_FILETYPE_PEM);
    int prikey_fp = SSL_CTX_use_PrivateKey_file(ctx, privkey_path.c_str(), SSL_FILETYPE_PEM);
    bool key_check = SSL_CTX_check_private_key(ctx);
    if (cert_fp <= 0 || prikey_fp <= 0 || !key_check)
    {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    s = socket(AF_INET, SOCK_STREAM, 0);
    int timeout = 10;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &timeout, sizeof(timeout));
    if(s == -1)
    {
        logging::ERROR("cannot open socket");
        return -1;
    }
    logging::INFO("socket opened: " + std::to_string(s));

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
        logging::ERROR("bind failed");
        return -1;
    }
    logging::INFO("bind success");
    int listen_res = listen(s, 10);
    if(listen_res == -1)
    {
        logging::ERROR("listen failed");
        return -1;
    }
    logging::INFO("listen client");
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
        fd.open(full_path, std::ios::in);
        fd.seekg(0, std::ifstream::end);
        int file_size =fd.tellg();
        fd.seekg(0, std::ifstream::beg);
        response->response_header["Content-Length"] = std::to_string(file_size);
        response->response_header["Connection"] = "keep-alive";
        response->body_length = file_size+3;
        char tmp = 0;
        int count = 0;
        char* tmp_body = new char[response->body_length];
        while ((tmp = fd.get())!=EOF)
        {
            // std::cout << tmp;
            memset(&(tmp_body[count]), tmp, 1);
            count++;
        }
        memset(&(tmp_body[count]), '\r', 1);
        memset(&(tmp_body[count+1]), '\n', 1);
        memset(&(tmp_body[count+2]), '\0', 1);
        response->send_queue.push_back(tmp_body);
        fd.close();
    }
    else if(type == "mp4")
    {
        send_type = MP4;
        std::string full_path = std::string(ROOT_PATH) + request->url; // path in server
        logging::INFO("mp4 path: " + full_path);

        FILE* video_fp;
        if((video_fp = fopen(full_path.c_str(), "rb")) == NULL)
        {
            logging::ERROR("open mp4 error");
            exit(-1);
        }
        fseek(video_fp, 0, SEEK_END);
        int video_length = ftell(video_fp);

        response->response_header["Content-Type"] = "video/mpeg";
        response->response_header["Accept-Ranges"] = "bytes";
        response->response_header["Connection"] = "keep-alive";
        //response->response_header["Transfer-Encoding"] = "chunked"; // conflict with Content-Length
        response->response_header["Content-Length"] = std::to_string(video_length);

        if(response->code == 200)
        {
            while(true)
            {
                logging::DEBUG("received 200 code");
                break;
            }
        }
        else
        {
            int range_length = request->request_header["Range"].length();
            std::string request_range = request->request_header["Range"].substr(6, range_length-6);
            int hyphen_pos = request_range.find("-");
            std::string start_string = request_range.substr(0, hyphen_pos);
            std::string end_string = "";
            // cout << "hyphen pos " << hyphen_pos << " " << request_range.length() << endl;
            logging::DEBUG("hyphen pos " + std::to_string(hyphen_pos) + " " +
                            std::to_string(request_range.length()));
            if(hyphen_pos != request_range.length()-1)
            {
                end_string = request_range.substr(hyphen_pos+1);
            }
            long start_addr = -1, end_addr = -1;
            logging::DEBUG("range " + request_range);

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
                logging::ERROR("range error");
                return;
            }
            long read_length = end_addr - start_addr + 1;
            response->body_length = read_length+3;

            std::ostringstream content_range;
            content_range << "bytes " << start_addr << "-" << end_addr << "/" << video_length;
            response->response_header["Content-Length"] = std::to_string(read_length);
            response->response_header["Content-Range"] = content_range.str();
            // std::cout << "reading: " << start_addr << " to " << end_addr << std::endl;
            logging::DEBUG("reading: " + std::to_string(start_addr) + " to " + std::to_string(end_addr));
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
        }
    }
}

void Server::accept_request()
{
    response = new HttpResponse;
    request = new HttpRequest;
    struct sockaddr_in client_addr;
    int len = sizeof(client_addr);
    int client_fd = accept(s, (struct sockaddr*)&client_addr, (socklen_t*)&len); // blocked waiting
    // std::cout << errno << std::endl;
    if(client_fd == -1)
    {
        close(client_fd);
        delete request;
        delete response;
        return;
    }
    char* ip = inet_ntoa(client_addr.sin_addr);
    std::string ip_str = ip;
    logging::INFO("connect to " + ip_str);

    SSL* ssl = SSL_new(ctx);
    memset(buf, 0, BUF_SIZE);
    if(mode == HTTP_MODE)
    {
         int size = read(client_fd, buf, sizeof(buf));
    }
    else
    {
        SSL_set_fd(ssl, client_fd);
        if (SSL_accept(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
        }
        int size = SSL_read(ssl, buf, BUF_SIZE);
    }
    request->string2header(buf);
    response-> ver_res = request->ver;

//    std::cout << "url " << request->url << std::endl;
//    std::cout << "path " << ROOT_PATH + request->url << std::endl;
//    std::cout << "access " << access((ROOT_PATH + request->url).c_str(), F_OK) << std::endl;
    logging::DEBUG("url " + request->url);
    logging::DEBUG("access " + std::to_string(access((ROOT_PATH + request->url).c_str(), F_OK)));

    err_flag = false;
    redirect_flag = false;
    if(access((ROOT_PATH + request->url).c_str(), F_OK) < 0)
    {
        logging::WARN("Not found");
        response->code = 404;
        err_flag = true;
    }
    else if(access((ROOT_PATH + request->url).c_str(), R_OK) < 0)
    {
        logging::WARN("Forbidden");
        response->code = 403;
        err_flag = true;
    }
    else if(mode == HTTP_MODE) // turn to https, not complemented yet
    {
        logging::INFO("Moved Permanently");
        response->code = 301;
        response->response_header["Content-Type"] = "text/html";
        response->response_header["Connection"] = "keep-alive";
        std::string location = "https://";
        location += ip;
        location += request->url;
        response->response_header["Location"] = location;
        redirect_flag = true;
    }
    else
    {
        int dot_pos = request->url.find(".");
        if(dot_pos == std::string::npos)
        {
            logging::WARN("Not found: url don't have file type");
            response->code = 404;
            err_flag = true;
        }
        else
        {
            std::string file_type = request->url.substr(dot_pos+1, request->url.length()-dot_pos);
            logging::INFO(file_type + " file will transmit to client");
            if(file_type == "mp4" && request->request_header.find("Range") != response->response_header.end())
            {
                logging::INFO("Partial Content");
                response->code = 206;
            }
            else
            {
                logging::INFO("OK");
                response->code = 200;
            }

            set_response_var(file_type);
        }
    }
    response->state = STATE_CODE_MAP[response->code];
    response->ver_res = "HTTP/1.1";
    std::string message = response->header2string(send_type);
    if(send_type == HTML) // a confusing bug
    {
        message += CRLF;
    }

    if(redirect_flag)
    {
        send(client_fd, message.c_str(), message.length(), MSG_WAITALL);
        logging::DEBUG("Redirect\n" + message);
        return;
    }
    logging::DEBUG(message);

    if(err_flag)
    {
        logging::WARN("error occurred, connection close");
        SSL_free(ssl);
        close(client_fd);
        delete request;
        delete response;
        return;
    }
    SSL_write(ssl, message.c_str(), message.length());
    if(send_type == MP4)
    {
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
            int error_code = -1;
            //error_code = send(client_fd, data, data_length, MSG_DONTWAIT);
            error_code = SSL_write(ssl, data, data_length);
            // std::cout << "packet: " << std::to_string(count) << " error code:" << error_code << std::endl;
            delete [] data;
        }
    }
    else if(send_type == HTML)
    {
        if(!response->send_queue.empty())
        {
            // send(client_fd, response->send_queue[0], response->body_length, MSG_WAITALL);
            SSL_write(ssl, response->send_queue[0], response->body_length);
        }
    }
    else
    {
        std::string err_message = "file type don't supported";
        err_message += "\r\n";
        // send(client_fd, err_message.data(), err_message.length(), MSG_WAITALL);
        SSL_write(ssl, err_message.data(), err_message.length());
    }
    response->send_queue.clear();

    if(SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN)
    {
        logging::WARN("SSL shutdown ignored");
    }
    else
    {
        SSL_shutdown(ssl);
    }

    SSL_free(ssl);
    close(client_fd);
    // SSL_CTX_free(ctx);
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
