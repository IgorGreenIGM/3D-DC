#include <tuple>
#include <vector>
#include <string>
#include <fstream>
#include <cassert>
#include <iostream>
#include <algorithm>

int main(int argc, char* argv[]) 
{
    std::ifstream ifs(argv[1], std::ios::binary);
    ifs.unsetf(std::ios::skipws);

    std::vector<std::ofstream> ofss;
    for (int i = 0; i < 256; ++i)
    {
        ofss.push_back(std::move(std::ofstream("pos/" + std::to_string(i) + ".txt", std::ios::binary)));
        ofss[i].unsetf(std::ios::skipws);
    }

    uint8_t c;   
    std::vector<std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>>> poss(256, std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>>());
    int i = 0, r = 0, div_grid = 256*256*256, div_mat = 256*256, div_line = 256;
    while (ifs >> c)    
    {
        int n_grid = 0, n_mat = 0, n_line = 0, n_col = 0;
        n_grid = i / div_grid; 
        r = i - div_grid*n_grid;
        n_mat = r / div_mat; 
        r = r - (div_mat*n_mat);
        n_line = r / div_line;
        r = r - (div_line*n_line);
        n_col = r%256;

        if (n_grid*div_grid + n_mat*div_mat + n_line*div_line + n_col != i)
            std::cerr << "err\n";

        assert(n_grid*div_grid + n_mat*div_mat + n_line*div_line + n_col == i);
        poss[(int)c].push_back(std::tie(n_grid, n_mat, n_line, n_col));        
        ++i;
    }

        // for (auto& pos : poss) {
        //     std::stable_sort(pos.begin(), pos.end(), [](const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>& p1, const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>& p2) {
        //                         return std::get<0>(p1) < std::get<0>(p2);
        //                     });

        //     std::stable_sort(pos.begin(), pos.end(), [](const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>& p1, const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>& p2) {
        //                         return std::get<1>(p1) < std::get<1>(p2);
        //                     });

        //     std::stable_sort(pos.begin(), pos.end(), [](const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>& p1, const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>& p2) {
        //                         return std::get<2>(p1) < std::get<2>(p2);
        //                     });

        //     std::stable_sort(pos.begin(), pos.end(), [](const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>& p1, const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>& p2) {
        //                         return std::get<3>(p1) < std::get<3>(p2);
        //                     });
        // }

    // for (auto& pos : poss)
    //     std::sort(pos.begin(), pos.end(), [](const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>& p1, const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>& p2) 
    //                                         {
    //                                             if (std::get<3>(p1) == std::get<3>(p2)) 
    //                                                 if (std::get<1>(p1) == std::get<1>(p2)) 
    //                                                     if (std::get<2>(p1) == std::get<2>(p2))
    //                                                         return std::get<0>(p1) < std::get<0>(p2);
    //                                                     else
    //                                                         return std::get<2>(p1) < std::get<2>(p2);
    //                                                 else
    //                                                     return std::get<1>(p1) < std::get<1>(p2);
    //                                             else
    //                                                 return std::get<3>(p1) < std::get<3>(p2);
    //                                         });

    for (int i = 0; i < 256; ++i)
        for (const auto& t : poss[i])
            ofss[i] << (int)std::get<3>(t) << "-" << (int)std::get<2>(t) << "-" << (int)std::get<1>(t) << std::endl;

    for (int i = 0; i < 256; ++i)
        ofss[i].close();

    ifs.close();
}