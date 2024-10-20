
#include <iostream>
#include "Socket.hpp"

using namespace std;

int main(void)
{
    try
    {
        Socket::MFileServer sock;
        sock.start_service("recv_file", "file2");  
    }
    catch (Socket::MException &e)
    {
        cout << e.what() << endl;
    }
    
    return 0;
}
