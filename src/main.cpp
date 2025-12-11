#include <ctime>
#include <string>
#include <chrono>
#include <random>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <sys/stat.h>

#include "../include/UnaryTests.hpp"
#include "../include/Dcompress/DCMap.hpp"
#include "../include/Dcompress/DCQueue.hpp"


using namespace std::literals;

unsigned int combinaison(int k, int n)
{
    int i, j;
    auto b = new unsigned int[n+1];
    b[0] = 1;
    for (i = 1; i <= n; i++)
    {
        b[i] = 1;
        for (j = i-1; j > 0; --j) 
            b[j] = b[j] + b[j-1];
    }
    
    auto res = b[k];
    delete[] b;
    return res;
}

unsigned int combinaison2(int k, int n)
{
    int i, j;
    auto b = new unsigned int[2*n];
    b[0] = 1;
    for (i = 1; i <= n; i++)
    {
        b[i] = 1;
        for (j = i-1; j > 0; --j) 
            b[j] = b[j] + b[j-1];
    }
    
    auto res = b[k];
    delete[] b;
    return res;
}


int main(int argc, char *argv[])
{
    // std::ofstream bin_out("test-256x.bin", std::ios::binary);
    // for (int i = 0; i < 256*256*256; ++i)
    //     bin_out << (uint8_t)(rand()%255);

    // argv[1] = file path + name, argv[2] = matrix size, argv[3] = queue size, argv[4] = buffer size
    struct stat st;
    stat(argv[1], &st);
    std::cout << "Starting building Buffer . . ." << std::endl;
    DCBuffer buf(argv[1], st.st_size);
    std::cout << "building Buffer completed." << std::endl;

    std::cout << "Starting building Queue . . ." << std::endl;
    DCQueue queue(buf, 50, 256);
    auto nb = queue.build(2, 2);
    std::cout << "building Queue completed : " << nb << " matrix was built"<< std::endl;

    // std::cout << "ENTROPY COMPUTING TEST : \n";
    // std::cout << "Starting by 3D==============================================================================\n";
    // queue._3D_filter();
    // std::cout << "3D : entropy -> " << queue.entropy() << "\n";
    // queue._2D_filter();
    // std::cout << "3D-2D : entropy -> " << queue.entropy() << "\n";
    // queue._3D_filter();
    // std::cout << "3D-2D-3D : entropy -> " << queue.entropy() << "\n";
    // queue._2D_filter();
    // std::cout << "3D-2D-3D-2D : entropy -> " << queue.entropy() << "\n";

    // std::cout << "Starting by 2D==============================================================================\n";
    // queue._2D_filter();
    // std::cout << "2D : entropy -> " << queue.entropy() << "\n";
    // queue._3D_filter();
    // std::cout << "2D-3D : entropy -> " << queue.entropy() << "\n";
    // queue._2D_filter();
    // std::cout << "2D-3D-2D : entropy -> " << queue.entropy() << "\n";
    // queue._3D_filter();
    // std::cout << "2D-3D-2D-3D : entropy -> " << queue.entropy() << "\n";

    std::cout << "finished filtering ....\n";

    std::cout << "\n\n----------------------------------------------------------------------;\nInitial\n";
    for (const auto &r : queue)
        std::cout << r.first << " | ";

    std::cout << "queue sorting . . .\n";
    std::cout << "\n\n----------------------------------------------------------------------;\nAfter\n";
    // queue.sort();
    for (const auto &r : queue)
        std::cout << r.first << " | ";

    // std::ofstream bin_out("out_out_filtered_sorted.bin", std::ios::binary);
    // bin_out.unsetf(std::ios::skipws);
    // const auto& out {queue.parse(PARSER::LINEAR_X)};
    // std::copy(out.begin(), out.end(), std::ostream_iterator<uint8_t>(bin_out));


    std::ofstream bin_out1("out_out_filtered_sorted.bin", std::ios::binary);
    bin_out1.unsetf(std::ios::skipws);
    DCMap map(queue);
    std::vector<uint8_t> xx, yy, zz;
    for (int i = 0; i < 256; ++i)
    {
        const auto& points = map.eq_points((uint8_t)i);
        
        auto mid = (uint8_t)std::ceil(queue.get_queue_size()/2.0);
        for (const auto& point : points) 
        {
            auto x = (uint8_t)point.get_x();
            auto y = (uint8_t)point.get_y();
            auto z = (uint8_t)point.get_z();

            if (x > 128)
                x -= 128;
            xx.push_back(x);

            if (y > 128)
                y -= 128;
            yy.push_back(y);

            if (z > mid)
                z -= mid;
            zz.push_back(z);
        }

        // for (const auto& point : points)
        // {
        //     auto y = (uint8_t)point.get_y();
        //     // if (y > 128)
        //         // y -= 128;
        //     yy.push_back(y);

        // }

        // for (const auto& point : points)
        // {
        //     auto z = (uint8_t)point.get_z();
        //     // if (z > mid)
        //         // z -= mid;
        //     zz.push_back(z);
        // }
    }

    // std::copy(xx.begin(), xx.end(), std::ostream_iterator<uint8_t>(bin_out1));
    // std::copy(yy.begin(), yy.end(), std::ostream_iterator<uint8_t>(bin_out1));
    // std::copy(zz.begin(), zz.end(), std::ostream_iterator<uint8_t>(bin_out1));

    // std::cout << "initial file distribution : " << queue.sd() << std::endl;
    // queue._3D_filter();

    // std::vector<std::pair<int, double>> sds;
    // for (int i = 10; i < 500; ++i)
    // {
    //     DCBuffer buf(argv[1], st.st_size);
    //     std::cout << "building Buffer completed." << std::endl;
        
    //     std::cout << "Starting building Queue . . ." << std::endl;
    //     DCQueue queue(buf, i, i);
    //     auto nb = queue.build(2, 2);
    //     std::cout << "building Queue completed : " << nb << " matrix was built"<< std::endl;

    //     std::cout << "initial file distribution : " << queue.sd() << std::endl;

    //     queue._3D_filter();
    //     double sd = queue.sd();
    //     sds.push_back({i, sd});
    //     std::cout << "file distribution after 3D filtering : " << sd << std::endl;

    //     std::ofstream bin_out("million/"s + std::to_string(i) + ".bin", std::ios::binary);
    //     bin_out.unsetf(std::ios::skipws);
    //     const auto& out {queue.parse(PARSER::LINEAR_X)};
    //     std::copy(out.begin(), out.end(), std::ostream_iterator<uint8_t>(bin_out));
    //     std::cout << "----------------------------------------------------------------\n";
    // }

    // std::sort(sds.begin(), sds.end(), [&](const std::pair<int, double>&a, const std::pair<int, double>&b) {return a.second > b.second;});
    // for (const auto& couple : sds)
    //     std::cout << "matrix size : " << couple.first << " standart deviation : " << couple.second << std::endl;

    // std::cout << "\n\n maximum standart deviation : " << sds.at(0).first << " with value " << sds.at(0).second << std::endl;
    // uint8_t c;
    // std::vector<uint8_t> vec;
    // for (int i = 0; i < 10000000; ++i)
    //     vec.push_back(0);
    // auto rng = std::default_random_engine {};
    // std::ifstream ifs(argv[1], std::ios::binary);
    // while (ifs >> c)
    //     vec.push_back(c);    
    
    // std::ofstream ofs1("shuffle-1--.bin", std::ios::binary);
    // std::copy(vec.begin(), vec.end(), std::ostream_iterator<uint8_t>(ofs1));
    
    // auto str = ""s;
    // bool next_is_3D = true;
    // for (int i = 0; i < 100; ++i)
    // {
    //     bool choice = static_cast<bool>(rand()%2);
    //     if (choice == 0 and next_is_3D)
    //     {
    //         queue._3D_filter();
    //         str += "-3D";
    //         std::cout << "file distribution after " << str << " filtering : " << queue.sd() << std::endl;   
    //         next_is_3D = false;
    //     }
    //     else
    //     {
    //         queue._2D_filter();
    //         str += "-2D";
    //         std::cout << "file distribution after " << str << " filtering : " << queue.sd() << std::endl; 
    //         next_is_3D = true;
    //     }
    // }
    

    // queue._2D_filter();
    // std::cout << "file entropy after 2D filtering : " << queue.entropy() << std::endl;
    
    // queue._3D_filter();
    // std::cout << "file entropy after 2D - 3D filtering : " << queue.entropy() << std::endl;

    // queue.unfilter();

    // queue._3D_filter();
    // std::cout << "file distribution after 3D filtering : " << queue.sd() << std::endl;

    // queue._2D_filter();
    // std::cout << "file distribution after 3D - 2D filtering : " << queue.sd() << std::endl;

    // queue._3D_filter();
    // std::cout << "file distribution after 3D filtering : " << queue.sd() << "| entropy : " << queue.entropy() << std::endl;

    // queue._2D_filter();
    // std::cout << "file distribution after 3D - 2D filtering : " << queue.sd() << "| entropy : " << queue.entropy() << std::endl;

    // queue._3D_filter();
    // std::cout << "file distribution after 3D - 2D - 3D filtering : " << queue.sd() << "| entropy : " << queue.entropy() << std::endl;
    
    // queue._2D_filter();
    // std::cout << "file distribution after 3D - 2D - 3D - 2D filtering : " << queue.sd() << "| entropy : " << queue.entropy() << std::endl;
    
    // queue._3D_filter();
    // std::cout << "file distribution after 3D - 2D - 3D - 2D - 3D filtering : " << queue.sd() << std::endl;

    // queue._2D_filter();
    // std::cout << "file distribution after 3D - 2D - 3D - 2D - 3D - 2D filtering : " << queue.sd() << std::endl;

    // queue.unfilter();
    // std::ofstream bin_out("out-256x-filtered-coords-st-all.bin", std::ios::binary);
    // bin_out.unsetf(std::ios::skipws);
    // const auto& out {queue.parse(PARSER::LINEAR_X)};
    // std::copy(out.begin(), out.end(), std::ostream_iterator<uint8_t>(bin_out));

    // std::ofstream ofs("out-256x-filtered-coords-st-all-2.bin", std::ios::binary);
    // std::ofstream x("out-256x-filtered-coords-st-x.bin", std::ios::binary);
    // std::ofstream y("out-256x-filtered-coords-st-y.bin", std::ios::binary);
    // std::ofstream z("out-256x-filtered-coords-st-z.bin", std::ios::binary);
    // x.unsetf(std::ios::skipws);
    // y.unsetf(std::ios::skipws);
    // z.unsetf(std::ios::skipws);
    // ofs.unsetf(std::ios::skipws);
    // DCMap map(queue);
    // for (int i = 0; i < 256; ++i)
    // {
    //     const auto& points = map.eq_points((uint8_t)i);
    //     for (const auto& point : points)
    //         ofs << (uint8_t)point.get_x();
    // }
    // for (int i = 0; i < 256; ++i)
    // {
    //     const auto& points = map.eq_points((uint8_t)i);
    //     for (const auto& point : points)
    //         ofs << (uint8_t)point.get_y();
    // }
    // for (int i = 0; i < 256; ++i)
    // {
    //     const auto& points = map.eq_points((uint8_t)i);
    //     for (const auto& point : points)
    //         ofs << (uint8_t)point.get_z();
    // }

    // std::ofstream bin_out1("out-million-filtered-coords-st.bin", std::ios::binary);
    // bin_out1.unsetf(std::ios::skipws);
    // DCMap map(queue);
    // for (int i = 0; i < 256; ++i)
    // {
    //     const auto& points = map.eq_points((uint8_t)i);
        
    //     for (const auto& point : points)
    //         if (point.get_x() >= (263/2))
    //             bin_out1 << (uint8_t)(point.get_x() - (263/2)); // ciblage sur une valeur spécifique
    //         else
    //             bin_out1 << (uint8_t)point.get_x();

    //     for (const auto& point : points)
    //         bin_out1 << (uint8_t)point.get_y();

    //     for (const auto& point : points)
    //             bin_out1 << (uint8_t)(point.get_z());
    // }

    // std::ofstream bin_out1("out-image-filtered-coords-st.bin", std::ios::binary);
    // bin_out1.unsetf(std::ios::skipws);
    // DCMap map(queue);
    // std::vector<uint8_t> xx, yy, zz;
    // for (int i = 0; i < 256; ++i)
    // {
    //     const auto& points = map.eq_points((uint8_t)i);
        
    //     for (const auto& point : points)
    //         xx.push_back((uint8_t)point.get_x());

    //     for (const auto& point : points)
    //         yy.push_back((uint8_t)point.get_y());

    //     for (const auto& point : points)
    //         zz.push_back((uint8_t)point.get_z());
    // }

    // std::copy(xx.begin(), xx.end(), std::ostream_iterator<uint8_t>(bin_out1));
    // std::copy(yy.begin(), yy.end(), std::ostream_iterator<uint8_t>(bin_out1));
    // std::copy(zz.begin(), zz.end(), std::ostream_iterator<uint8_t>(bin_out1));

    // std::ofstream o1("before.txt");
    // const auto &vec1 = queue.parse(PARSER::LINEAR_X);
    // for (int i = 0; i < 256; ++i)
    //     o1 << i << " " << std::count(vec1.begin(), vec1.end(), (uint8_t)i) << "\n";

    // std::cout << "Starting 3D filtering . . ." << std::endl;
    // queue._3D_filter();
    // std::cout << "3D filtering completed" << std::endl;
    // queue._2D_filter();

    // std::ofstream o2("after.txt");
    // const auto &vec2 = queue.parse(PARSER::LINEAR_X);
    // for (int i = 0; i < 256; ++i)
    //     o2 << i << " " << std::count(vec2.begin(), vec2.end(), (uint8_t)i) << "\n";

    // std::ofstream o("out_"s + argv[1], std::ios::binary);

    // std::cout << "Started saving filtered file . . ." << std::endl;
    // const auto &vec = queue.parse(PARSER::LINEAR_X);
    // std::copy(vec.begin(), vec.end(), std::ostream_iterator<uint8_t>(o));
    // std::cout << "Saving filtered file finished" << std::endl;

    return 0;
};