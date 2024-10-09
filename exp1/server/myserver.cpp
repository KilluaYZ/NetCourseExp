
#include <iostream>
#include "Socket.hpp"

using namespace std;

int main(void)
{
    try
    {
        Socket::FileSocket sock;
        sock.bind(2000);
        sock.recv("127.0.0.1", 3000, "/home/killuayz/NetCourseExp/exp1/server/file_recv");
        sock.close();
    }
    catch (Socket::Exception &e)
    {
        cout << e.what() << endl;
    }
    
    return 0;
}
