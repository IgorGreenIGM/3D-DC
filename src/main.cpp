#include <iostream>
#include <sys/stat.h>

#include "../include/UnaryTests.hpp"
#include "../include/Dcompress/DCQueue.hpp"


int main(int argc, char *argv[])
{
    // argv[1] = file path + name, argv[2] = matrix size, argv[3] = queue size, argv[4] = buffer size
    struct stat st;
    stat(argv[1], &st);
    DCQueue::filter_file(argv[1], "output.fltred", PARSER::LINEAR_Z, std::stoi(argv[3]), std::stoi(argv[2]), std::stoi(argv[4]));

    return 0;
};