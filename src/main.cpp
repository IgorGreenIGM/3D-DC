#include <iostream>
#include <iterator>
#include <string>
#include <random>
#include <sys/stat.h>

#include "../include/UnaryTests.hpp"
#include "../include/Dcompress/DCMap.hpp"
#include "../include/Dcompress/DCQueue.hpp"
#include <ctime>


int main(int argc, char *argv[])
{
    srand(time(nullptr));

    // argv[1] = file path + name, argv[2] = matrix size, argv[3] = queue size, argv[4] = buffer size
    struct stat st;
    stat(argv[1], &st);
    // DCQueue::filter_file(argv[1], "output.fltred", PARSER::LINEAR_Z, std::stoi(argv[3]), std::stoi(argv[2]), std::stoi(argv[4]));
    DCBuffer buf(argv[1], st.st_size);
    DCQueue queue(buf, 10, 10);
    auto nb = queue.build(2, 2);

    DCMap map(queue);
    std::ofstream o("out.txt");
    const auto &v = map.eq_neighbours({2,2,2}, 15000);
    std::cout << v.size() << "\n";
    for (const auto &val : v)
        o << val << " ";
    
    // const auto &v = map.range_parse({0, 0, 0}, {0, queue.get_matrix_nb() - 1, 0});
    return 0;
};