#ifndef MEXCEPTION_HPP
#define MEXCEPTION_HPP
#include <string>
#include <iostream>
using namespace std;

class MException
    {
    private:
        string _message;

    public:
        MException(string error) { this->_message = error; }
        virtual const char *what() { return this->_message.c_str(); }
    };

#endif