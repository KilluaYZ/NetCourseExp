
#include <iostream>
#include <csignal>
#include "Socket.hpp"

using namespace std;

Socket::MFileServer sock;

void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        cout << "Caught Ctrl+C Exiting..." << endl;
        sock.close();
        exit(0);
    }
}

int main(void)
{
    signal(SIGINT, signalHandler);
    int n;
    cout << "[1] : send msg" << endl << "[2] : send file" << endl << "Please input: ";
    cin >> n;
    try
    {
        if(n == 2)
        sock.start_service("file2", "recv_file");
        else if(n == 1) 
        sock.start_msg_service();
    }
    catch (exception &e)
    {
        cout << e.what() << endl;
        sock.close();
    }
    return 0;
}
