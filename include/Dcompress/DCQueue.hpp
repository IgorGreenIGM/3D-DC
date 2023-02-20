#ifndef _DC_QUEUE_H_INCLUDED_
#define _DC_QUEUE_H_INCLUDED_

#include <map>
#include <vector>
#include <cmath>

#include "./DCMatrix.hpp"
#include "./DCBuffer.hpp"
/**
 * @brief class DCQueue
 * @note this class is the entry point of the compression / filtering.
 * @note It what specifically build to work with DCBUffer so it only accept constructor from DCBuffer.
 * @see DCBuffer
 */
class DCQueue : public std::vector<std::pair<int, DCMatrix>>
{
    public : 
        DCQueue(DCBuffer &buffer, int queue_size, int matrix_size);

        double entropy() const noexcept;
        void swap(int id_1, int id_2);
        std::size_t build(double buf_delay, double queue_delay) noexcept; 
        std::size_t last_data_size() const noexcept;
        std::vector<uint8_t> get_all_z() const noexcept;
        std::vector<uint8_t> get_all_linear() const noexcept;
        std::vector<uint8_t> filter(bool _2D, bool _3D) noexcept;
        std::vector<uint8_t> get_z(int line, int col) const noexcept;
        void unfilter(const std::vector<uint8_t> &filters_dict, bool _2D, bool _3D);
        std::vector<uint8_t> get_linear(bool filtred, bool unfiltred) const noexcept;

        static void unfilter_file(const std::string&, const std::string&, const std::string&, double buf_delay = 1, double queue_delay = 1); // [BETA]
        static void filter_file(const std::string&, const std::string&, const std::string&, int, int, int, bool, bool, double buf_delay = 1, double queue_delay = 1); // [BETA]

    private :
        int counter; // counter of builded matrix
        int queue_size; // size of the queue
        int matrix_size; // size of the matrix
        DCBuffer &buf_ref; // reference of the working-with buffer
};

// never, never change the order of this enumaration
enum FILTER_MODE {NO_2D, SUB_2D, UP_2D, AVERAGE_2D, PAETH_2D, // 2D filters ids
                  NO_3D, SUB_3D, UP_3D, AVERAGE_3D, PAETH_3D // 3D filters ids
                 };


inline bool is_big_endian()
{
    int nb = 1;
    auto *ptr = reinterpret_cast<uint8_t*>(&nb);

    return (ptr[0] == 0);
}

// Paeth filter algorithm
inline uint8_t paeth_pred(uint8_t left, uint8_t up, uint8_t upperLeft)
{
    int p = (left + up - upperLeft);
    int p_left = std::abs(p - left), p_up = std::abs(p - up), p_upperLeft = std::abs(p - upperLeft);
    if (p_left <= p_up && p_left <= p_upperLeft)
        return left;
    else if (p_up <= p_upperLeft)
        return up;
    else
        return upperLeft;
};

// cardinality compute
inline std::size_t cardinality(const std::vector<uint8_t> &vec)
{
    std::vector<uint8_t> computed;
    for(int i = 0; i < vec.size(); ++i)
    {
        bool compute = true;
        for(auto value : computed)
            if(vec[i] == value)
            {
                compute = false;
                break;
            }
        if(compute)
            computed.push_back(vec[i]);
    }
    return computed.size();
}

// entroy compute
inline double _entropy(const std::vector<uint8_t> &vec)
{
    std::map<std::size_t, int> frequency;
    for (auto byte : vec)
        ++frequency[byte];

    double _entropy = 0, length = static_cast<double>(vec.size());
    for (const auto &freq : frequency)
    {
        double p_x = static_cast<double>(freq.second) / length;
        _entropy -= p_x * std::log2f(p_x);  
    }

    return _entropy;
}

#endif // _DC_QUEUE_H_INCLUDED_