#include <iostream>
#include "server.h"

int main()
{
    std::cout << "test\n" << std::endl;
    Server s1(HTTP_MODE);
    if(s1.set_socket()<0)
    {
        return -1;
    }
    while (1)
    {
        s1.accept_request();
    }
    return 0;
}