#include <iostream>
#include <thread>
#include "server.h"


int main()
{
    std::cout << "This is a simple http/https server written by Tong Bohan, UCAS" << std::endl;
    //logging::configure({ {"type", "file"}, {"file_name", "server_log.log"}, {"reopen_interval", "1"} });
    logging::INFO("Server start");
    Server s1(HTTPS_MODE);
    Server s2(HTTP_MODE);
    if(s1.set_socket() < 0|| s2.set_socket() < 0)
    {
        logging::INFO("Socket set error");
        return -1;
    }
    while (true)
    {
        std::thread t1(&Server::accept_request, s1);
        std::thread t2(&Server::accept_request, s2);
        t1.join();
        t2.detach();
    }
    return 0;
}