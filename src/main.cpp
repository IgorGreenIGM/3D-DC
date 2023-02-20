#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <limits>

#include <thread>
#include "../include/UnaryTests.hpp"
#include "../include/Dcompress/DCQueue.hpp"

void evaluate_entropy(const std::string &file_1, const std::string &file_2)
{
    using namespace std::literals;

    std::ifstream ofs1(file_1, std::ios::binary);
    std::ifstream ofs2(file_2, std::ios::binary);

    std::vector<uint8_t> datas_1, datas_2;
    uint8_t tmp;

    while(ofs1 >> tmp)
        datas_1.push_back(tmp);
    
    while (ofs2 >> tmp)
        datas_2.push_back(tmp);

    std::cout << "entropy of file : "s + file_1 + " " + std::to_string(_entropy(datas_1)) << " entropy of file : "s + file_2 + " " + std::to_string(_entropy(datas_2)) << "\n";
}

int main(int argc, char *argv[])
{
    // argv[1] = file path + name, argv[2] = matrix size, argv[3] = queue size, argv[4] = 2D, argv[5] = 3D
    struct stat st;
    stat(argv[1], &st);

    auto sc = std::chrono::steady_clock();
    auto start = sc.now();

    std::cout << "file size :  " << st.st_size << "\n";

    std::ofstream o("_3D_.txt");
    for (int mat_size = 2; mat_size <= 1000; ++mat_size)
    {
        DCBuffer buf(argv[1], st.st_size);
        DCQueue queue(buf, 10, mat_size);
        queue.build(2, 2);
        queue.filter(false, true);

        o << mat_size << " " << queue.entropy() << std::endl;
        std::cout << "sate : " << mat_size << " / 1000" << "\n";
    }


    std::cout << "time elapsed : " << static_cast<std::chrono::duration<double>>(sc.now() - start).count() << "\n";

    return 0;
};