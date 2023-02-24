#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <sys/stat.h>
#include <algorithm>
#include <limits>

#include <thread>
#include "../include/UnaryTests.hpp"
#include "../include/Dcompress/DCQueue.hpp"
using namespace std::literals;
int main(int argc, char *argv[])
{
    // argv[1] = file path + name, argv[2] = matrix size, argv[3] = queue size, argv[4] = 2D, argv[5] = 3D
    struct stat st;
    stat(argv[1], &st);

    DCBuffer buf(argv[1], st.st_size);
    DCQueue queue(buf, 10, 100);
    queue.build(2, 2);

    for (int i = 0; i < 1; ++i)
    {
        queue._3D_filter();
        // queue._2D_filter();
    }

    std::ofstream o("out_"s + argv[1], std::ios::binary);
    o.unsetf(std::ios::skipws);

    const auto &v = queue.z_parse();
    std::copy(v.begin(), v.end(), std::ostream_iterator<uint8_t>(o));

    return 0;
};