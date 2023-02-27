#ifndef _DC_QUEUE_H_INCLUDED_
#define _DC_QUEUE_H_INCLUDED_

#include <map>
#include <cmath>
#include <vector>

#include "./DCMatrix.hpp"
#include "./DCBuffer.hpp"

/**
 * @brief parsers types of the DCQueue
 * @see DCQueue::parse()
 */
enum class PARSER
{
    LINEAR_X, LINEAR_Y, LINEAR_Z    
};

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

        // filtering 

        void unfilter();
        void _2D_filter();
        void _3D_filter();
        void clear_filters_dict();
        const std::vector<uint32_t> get_filters_dict();
        void set_filters_dict(const std::string &dict_path);
        void set_filters_dict(const std::vector<uint32_t> &dict_in);
        static void filter_file(const std::string &file_path, const std::string &dest_path, PARSER parser, int queue_size, int matrix_size, int buffer_size);

        // parsing

        std::vector<uint8_t> parse(PARSER parser) const;
        std::vector<uint8_t> linear_parse() const noexcept;
        std::vector<uint8_t> get_all_linear() const noexcept;
        std::vector<uint8_t> z_get(int line, int col) const noexcept;

        // building 

        void swap(int id_1, int id_2);
        double entropy() const noexcept;
        std::size_t last_data_size() const noexcept;
        std::size_t build(double buf_delay, double queue_delay) noexcept;

        // setters

        // return the number of build matrix in the Queue
        int get_matrix_nb()const noexcept
        {return this->counter;};
        // return the size of the Queue
        int get_queue_size() const noexcept
        {return this->queue_size;};
        // return the size of matrix inside the Queue
        int get_matrix_size() const noexcept
        {return this->matrix_size;};

    private :
        int counter; // counter of builded matrix
        int queue_size; // size of the queue
        int matrix_size; // size of the matrix
        DCBuffer &buf_ref; // reference of the working-with buffer

        std::vector<uint32_t> filters_dict; // dictionnary filter 
};

/** FILTER DOC
 * @warning cause we'll need to unfilter the Queue(in reversed other than filter), we write the group id at the end to dict to facilitate unfilter.
 * @warning then to unfilter ust need to read the last element which is the group id, decount number of written filters in the dict from end(according to filter group),
 * @warning and use this postion as filters ids start.
*/

// used to identify order of filtering methods(2D, 3D, etc)
enum FILTER_GROUP_ID 
{
    _2D_FILTER = 0x100,  // hex(256)
    _3D_FILTER
};

/**
 * @brief filters id's
 */
enum FILTER_MODE 
{ 
    NO_2D, SUB_2D, UP_2D, AVERAGE_2D, PAETH_2D, // 2D filters ids
    NO_3D, SUB_3D, UP_3D, AVERAGE_3D, PAETH_3D // 3D filters ids
};


/**
 * @brief check Hardware architecture endianess
 * 
 * @return true if big endian
 * @return false otherwise
 */
inline bool is_big_endian()
{
    int nb = 1;
    auto *ptr = reinterpret_cast<uint8_t*>(&nb);

    return (ptr[0] == 0);
}

/**
 * @brief Paeth filter algorithm
 * 
 * @param left left neighbour
 * @param up up neighbour
 * @param upperLeft upperleft neighbour 
 * @return uint8_t predicted value
 */
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


/**
 * @brief compute the cardinality of a vector
 * 
 * @param vec input vector
 * @return std::size_t cardinality 
 */
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

/**
 * @brief compute Shannon entropy of a vector
 * 
 * @param vec input vector
 * @return double computed entropy
 */
inline double _entropy(const std::vector<uint8_t> &vec)
{
    std::map<std::size_t, int> frequency;
    for (auto byte : vec)
        ++frequency[byte];

    double entropy = 0, length = static_cast<double>(vec.size());
    for (const auto &freq : frequency)
    {
        double p_x = static_cast<double>(freq.second) / length;
        entropy -= p_x * std::log2f(p_x);  
    }

    return entropy;
}

#endif // _DC_QUEUE_H_INCLUDED_