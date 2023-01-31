#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <algorithm>

#include "../include/Dcompress/DCMatrix.h"
#include "../include/Dcompress/DCBuffer.h"
#include "../include/Dcompress/DCQueue.h"

int main()
{
    DCBuffer buffer("./test.bin", 20);
    DCQueue queue(buffer, 5, 3);
    int nbre = queue.build(5, 5);

    auto sc = std::chrono::steady_clock();
    auto start = sc.now();
    for(int i = 0; i < 10000000; ++i)
        queue.swap(0, 1);
    std::cout << "with one-loop method : " << static_cast<std::chrono::duration<double>>(sc.now() - start).count() << "\n";
    // std::cout << "Before swaping : \n------------------------------------------------\n";
    // for(int i = 0; i < nbre + 1; ++i)
    // {
    //     queue[i].second.print();
    //     std::cout << "\n-----------------------------------------------\n"; 
    // }                                                                           //*/

    // std::cout << "After swapping : \n------------------------------------------------\n";
    // queue.swap(0, 1);
    // for(int i = 0; i < nbre + 1; ++i)
    // {
    //     queue[i].second.print();
    //     std::cout << "\n-----------------------------------------------\n"; 
    // }         

    /*std::ofstream out("./test.bin", std::ios::binary | std::ios::out);
    for(int i = 0; i < 59; ++i)
        out << std::noskipws << (uint8_t)i; //*/

    return 0;
}