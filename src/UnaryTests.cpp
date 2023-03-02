#include <algorithm>
#include <random>
#include "../include/DCompress/DCQueue.hpp"
#include "../include/DCompress/DCMap.hpp"

/*SUCEFULLY PASSED CORRECTED SOME BUGS DUE TO END MATRIX DATAS READ */
void test_if_build_method_works_to_get_all_datas(const std::string &file_path, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1)
{
    std::cout << "build unit test : ---------------------------------------\n";

    DCBuffer buffer(file_path, buffer_size); std::cout << "buffer init okk--\n";
    DCQueue queue(buffer, queue_size, mat_size); std::cout << "queue init okk--\n";

    auto build_nb = queue.build(del1, del2); std::cout << "queue build okk--\n";
    std::cout << "number of matrix build : " << build_nb << "\n";
    std::cout << "number of matrix in the queue : " << queue.size() << "\n";

    const auto &all = queue.linear_parse(); std::cout << "get linear all datas okk--\n";
    std::cout << "size of datas got from queue : " << all.size() << "\n";
    std::ofstream out("out_" + file_path, std::ios::binary);
    for (int i = 0; i < all.size(); ++i)
        out << std::noskipws << all[i];
    
    std::cout << "writing got datas okk--\n";

    std::system(("code --diff " + file_path + " " + "out_" + file_path).c_str());
    std::cout << "opening to see vs code difference okk--\n";
}

/*SUCEFULLY PASSED CORRECTED SOME BUGS DUE TO 2D FILTERS, METHOD WAS FILTERING END_MATRIX DATAS READ */
void test_if_filter_works_to_generate_filter_dictionnary(const std::string &file_path, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1)
{
    std::cout << "generate filter dictionnary output unit test : ---------------------------------------\n";

    DCBuffer buffer(file_path, buffer_size); std::cout << "buffer init okk--\n";
    DCQueue queue(buffer, queue_size, mat_size); std::cout << "queue init okk--\n";   

    auto build_nb = queue.build(del1, del2); std::cout << "queue build okk--\n";
    std::cout << "number of matrix build : " << build_nb << "\n";
    std::cout << "number of matrix in the queue : " << queue.size() << "\n";

    // const auto &fltrs = queue.filter(true, true);
    // std::cout << "size of the filters dictionnary output : " << fltrs.size() << "--\n";
    std::cout << "must be filters dictionnary output size : " << (build_nb * mat_size) + (mat_size * mat_size) << "--\n";

    // if (fltrs.size() != (build_nb * mat_size) + (mat_size * mat_size))
        // throw std::runtime_error("error when in filters size");
    
    std::cout << "finding the first 3D filter position, must be at : " << (build_nb * mat_size) << "(after 2D filter)\n";
    // auto found = std::find_if(fltrs.begin(), fltrs.end(), [](const uint8_t &v){return (int)v >= 5;});
    // if (std::distance(fltrs.begin(), found) != (build_nb * mat_size))
        // throw std::runtime_error("error in 3D filters positions");
    // else
        // std::cout << "3D filter position starts okk, found at position : " << std::distance(fltrs.begin(), found) << "--\n";

    std::ofstream out("out_filter_" + file_path, std::ios::binary);
    // for (const auto &fl : fltrs)
        // out << std::noskipws << fl;
    // std::cout << "generating filter dictionnary output okk--\n";
}

/*PASSED, fixed and rewrited multithreading in filter method*/
void test_if_filter_then_unfilter_works(const std::string &file_path, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1)
{
    std::cout << "test filter then unfilter unit test : ---------------------------------------\n";

    DCBuffer buffer(file_path, buffer_size); std::cout << "buffer init okk--\n";
    DCQueue queue(buffer, queue_size, mat_size); std::cout << "queue init okk--\n";

    auto build_nb = queue.build(del1, del2); std::cout << "queue build okk--\n";

    // queue._2D_filter();   
    queue._3D_filter();   
    std::cout << "filter queue okk--\n";

    std::cout << "matrix number inside the queue : " << build_nb << "--\n";
    queue.unfilter();
    std::cout << "unfilter queue okk--\n";

    const auto &datas = queue.linear_parse();
    std::cout << "retrieve datas okk--\n";

    std::cout << "all size : " << datas.size() << "\n";

    std::cout << "starting unfiltered output writing...\n";
    std::ofstream out("out_unfilter_" + file_path, std::ios::binary);
    for (const auto &v : datas)
        out << std::noskipws << v;
}


void filter_and_save_filtered_file(const std::string &file_path, bool _2D, bool _3D, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1)
{
    using namespace std::literals;
    std::cout << "filtering file  " + file_path << "-------------------\n";

    DCBuffer buffer(file_path, buffer_size); std::cout << "buffer init okk..........\n";
    DCQueue queue(buffer, queue_size, mat_size); std::cout << "queue init okk..........\n";

    auto build_nb = queue.build(del1, del2); std::cout << "queue build okk--\n";
    
    // queue.filter(_2D, _3D); std::cout << "filter queue okk..........\n";

    auto datas(queue.linear_parse()); std::cout << "retrieve datas okk..........\n";

    auto add = (_2D and _3D) ? "_2D_3D_"s : _3D ? "_3D_"s : "_2D_"s; 
    std::ofstream out("filtered_" + add + file_path, std::ios::binary);

    for (const auto &v : datas)
        out << std::noskipws << v;
}

/*NOT PASSED */
void test_if_unfilter_works_to_read_and_unfilters_datas(const std::string &file_path, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1)
{
    std::cout << "read and unfilter dictionnary datas unit test : ---------------------------------------\n";

    DCBuffer buffer(file_path, buffer_size); std::cout << "buffer init okk--\n";
    DCQueue queue(buffer, queue_size, mat_size); std::cout << "queue init okk--\n";   

    auto build_nb = queue.build(del1, del2); std::cout << "queue build okk--\n";
    std::cout << "number of matrix build : " << build_nb << "\n";
    std::cout << "number of matrix in the queue : " << queue.size() << "\n";

    uint8_t tmp(0);
    std::ifstream in("out_filter_" + file_path, std::ios::binary);

    std::vector<uint8_t>fltrs_in;
    while(in >> std::noskipws >> tmp)
        fltrs_in.push_back(tmp);
    
    std::cout << "reading filters dictionnary from filter dictionnary okk--\n";
    std::cout << "Starting unfilter\n";
    // queue.unfilter(fltrs_in, true, true);
    std::cout << "unfilter okk--\n";

    std::cout << "retrieving unfiltred datas from file : ...\n";
    const auto &all_datas = queue.linear_parse();
    std::cout << "retrieving unfiltred datas from file okk--\n";

    std::cout << "writing unfiltred datas into file...\n";
    std::ofstream out("out_unfilter_" + file_path, std::ios::binary);
    for (const auto &val : all_datas)
        out << std::noskipws << val;

    std::cout << "writing got datas okk--\n";
    std::system(("code --diff " + file_path + " " + "out_unfilter_" + file_path).c_str());
    std::cout << "opening to see vs code difference okk--\n";
}

void _2D_get(const std::string &file_path, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1)
{
    using namespace std::literals;
    std::cout << "2D filtering file  " + file_path << "-------------------\n";

    DCBuffer buffer(file_path, buffer_size); std::cout << "buffer init okk..........\n";
    DCQueue queue(buffer, queue_size, mat_size); std::cout << "queue init okk..........\n";

    auto build_nb = queue.build(del1, del2); std::cout << "queue build okk--\n";
    
    // queue.filter(true, false); std::cout << "2D filter queue okk..........\n";

    // auto datas(queue.z_parse()); std::cout << "retrieve datas in 2D okk..........\n";

    auto add = "_2D_"s;
 
    std::ofstream out("filtered_" + add + file_path, std::ios::binary);

    // for (const auto &v : datas)
        // out << std::noskipws << v;   
}

void _3D_get(const std::string &file_path, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1)
{
    using namespace std::literals;
    std::cout << "3D filtering file  " + file_path << "-------------------\n";

    DCBuffer buffer(file_path, buffer_size); std::cout << "buffer init okk..........\n";
    DCQueue queue(buffer, queue_size, mat_size); std::cout << "queue init okk..........\n";

    auto build_nb = queue.build(del1, del2); std::cout << "queue build okk--\n";
    
    // queue.filter(false, true); std::cout << "3D filter queue okk..........\n";

    // auto datas(queue.z_parse()); std::cout << "retrieve datas in 3D okk..........\n";

    auto add = "_3D_"s;
 
    std::ofstream out("filtered_" + add + file_path, std::ios::binary);

    // for (const auto &v : datas)
        // out << std::noskipws << v;   
}

double progressive_entropy(const std::string &file_name, int matrix_size, int buffer_size, int queue_size, bool _2D, bool _3D)
{
    // init buffer and queue
    DCBuffer buffer {file_name, buffer_size};
    DCQueue queue {buffer, queue_size, matrix_size};
    double _entropy = 0;
    while(true)
    {
        auto built_nb = queue.build(2, 2);
        // queue.filter(_2D, _3D);
        _entropy += queue.entropy();
        if (built_nb < queue_size)
            break;
    }

    return _entropy;
}

// test if to_local() and to_user() coord system change methods works(). 
void test_if_to_user_and_to_local_works(const DCMap &map)
{
    {
        DCPoint point {rand()%10, rand()%10, rand()%10};
        auto local = map.to_local(point);
        auto neww = map.to_user(local);

        if (point != neww)
            throw std::runtime_error("Error in unary test : comparison between localized and user-ed values are wrong");

        std::cout << "---------------------------------------------------------------------------------------------\n";
         std::cout << "Initial coords : " << point << "\n";
         std::cout << "changed coords : " << local << "\n"; 
         std::cout << "new coords : " << neww << "\n"; 
    }
}