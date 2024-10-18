
#include <iostream>
#include "Socket.hpp"

using namespace std;

int main(void)
{
    try
    {
        Socket::MFileServer sock;
        sock.bind(20000);
        sock.start_service("file2");  
        sock.close();
    }
    catch (Socket::Exception &e)
    {
        cout << e.what() << endl;
    }
    
    return 0;
}
