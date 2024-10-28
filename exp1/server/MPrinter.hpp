#ifndef MPRINTER_HPP
#define MPRINTER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <exception>
#include "MException.hpp"
using namespace std;

class MPrinter
{
private:
    int cur_x, cur_y;
    int max_x, max_y;
    int MAX_BUFFER_SIZE;
    int msg_max_row, msg_max_column;
    int last_x, last_y;
    vector<string> msg_buffer;
    void set_cursor_position(int x, int y)
    {
        this->last_x = this->cur_x;
        this->last_y = this->cur_y;
        this->cur_x = x;
        this->cur_y = y;
        cout << "\033[" << x + 1 << ";" << y + 1 << "H";
    }

    void _print_line()
    {
        for (int i = 0; i < this->max_y; i++)
            cout << "-";
        cout << endl;
    }

    void _print_line_with_space()
    {
        cout << "|";
        for (int j = 1; j < this->max_y - 1; j++)
            cout << " ";
        cout << "|" << endl;
    }

    void clear(){
        std::cout << "\033[2J\033[H" << std::flush;
    }

    void init()
    {
        clear();
        _print_line();
        for (int i = 1; i < this->max_x - 3; i++)
        {
            _print_line_with_space();
        }
        _print_line();
        _print_line_with_space();
        _print_line();

        this->msg_buffer.push_back("Welcome to use the terminal.");
        flush();
    }

    void flush()
    {
        clear_msg_box();
        int print_row_num = this->msg_max_row > this->msg_buffer.size() ? this->msg_buffer.size() : this->msg_max_row;
        for(int i = 0;i < print_row_num; i++){
            set_cursor_to_start_of_line(i);
            cout << this->msg_buffer[i];
        }
        set_cursor_to_last();
    }

    void clear_msg_box(){
        for(int i = 0; i < this->msg_max_row; i++){
            set_cursor_to_start_of_line(i);
            for(int j = 0;j < this->msg_max_column; j++) cout << " ";
        }
    }

    void push_and_swap(string msg)
    {
        this->msg_buffer.push_back(msg);
        if (this->msg_buffer.size() > this->MAX_BUFFER_SIZE)
        {
            this->msg_buffer.erase(this->msg_buffer.begin());
        }
    }

    void set_cursor(int row, int column){
        if(row < 0) throw MException("row can't < 0");
        if(column < 0) throw MException("column can't < 0");
        if(row >= this->msg_max_row) throw MException("row num overflow");
        if(column >= this->msg_max_column) throw MException("column num overflow");
        
        set_cursor_position(++row, ++column);
    }

    void set_cursor_to_input(){
        set_cursor_position(this->max_x - 2, 1);
    }

    void set_cursor_to_last(){
        set_cursor_position(this->last_x, this->max_y);
    }

    void set_cursor_to_start_of_line(int row){
        set_cursor(row,0);
    }

public:
    MPrinter(int max_x = 50, int max_y = 50)
    {
        this->max_x = max_x;
        this->max_y = max_y;
        this->cur_x = 0;
        this->cur_y = 0;
        this->MAX_BUFFER_SIZE = this->max_x - 4;
        this->msg_max_row = this->MAX_BUFFER_SIZE;
        this->msg_max_column = this->max_y - 2;
        init();
        set_cursor_to_input();
    }

    void print(string msg)
    {
        push_and_swap(msg);
        flush();
    }
};

#endif