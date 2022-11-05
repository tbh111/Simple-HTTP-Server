#include <iostream>
#include <thread>
#include "server.h"

int main()
{
    std::cout << "test\n" << std::endl;
    Server s1(HTTPS_MODE);
    Server s2(HTTP_MODE);
    if(s1.set_socket() < 0|| s2.set_socket() < 0)
    {
        return -1;
    }
    while (true)
    {
//        s2.accept_request();
//        s2.accept_request();
        std::thread t1(&Server::accept_request, s1);
        std::thread t2(&Server::accept_request, s2);
        t1.join();
        t2.detach();
        std::cout << "loop end" << std::endl;
    }
    return 0;
}