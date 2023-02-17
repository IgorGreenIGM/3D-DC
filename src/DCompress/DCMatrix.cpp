#include <cmath>
#include <algorithm>

#include "../../include/Dcompress/DCMatrix.h"

DCMatrix::DCMatrix()
{
    this->size = 0;
    this->col_disp_count = 0;
    this->line_disp_count = 0;
    this->disp_limit = 0;
}

/**
 * @brief Construct a new DCMatrix::DCMatrix object, from initializer_list
 * 
 * @param list initializer_list
 * @exception std::runtime_error() if the size of the initializer_list is not a perfect square.
 */
DCMatrix::DCMatrix(const std::initializer_list<uint8_t> &list)
{    
    if(std::ceil(std::sqrt(list.size())) != std::floor(std::sqrt(list.size()))) // checking if perfect square...
        throw std::runtime_error("Invalid size in constructor, must be a perfect square");

    this->col_disp_count = 0;
    this->line_disp_count = 0;
    this->size = static_cast<int>(std::sqrt(list.size()));
    this->disp_limit = fact(this->size); // limit for disposition counters

    for(auto it = list.begin(); it != list.end(); ++it) // copying initializer_list elements into the matrix
    {
        std::vector<uint8_t> tmp;
        for(int j = 0; j < this->size; ++j)
        {
            tmp.push_back(*it);
            ++it;
        }
        --it;
        this->push_back(tmp);
        tmp.clear();
    }
}

/**
 * @brief Construct a new DCMatrix::DCMatrix object, from input matrix
 * 
 * @param matrix input matrix
 * @exception std::runtime_error() if number of lines in the matrix is not equals to number of columns.
 */
DCMatrix::DCMatrix(const std::vector<std::vector<uint8_t>> &matrix)
{
    this->col_disp_count = 0;
    this->line_disp_count = 0;
    this->size = matrix.size();
    this->disp_limit = fact(this->size); // limit for disposition counters

    for(auto &line : matrix)
        if (this->size != line.size())
            throw std::runtime_error("Invalid size in constructor, number of lines must be equals to number of columns");

    for(auto &line : matrix)
        this->push_back(line);
}


/**
 * @brief Construct a new DCMatrix::DCMatrix object, from input buffer
 * 
 * @param buffer input buffer
 * @param buf_size input buffer size
 * @exception std::runtime_error() if the size of the buffer is not a perfect square.
 */
DCMatrix::DCMatrix(uint8_t *buffer, int buf_size)
{    
    if(std::ceil(std::sqrt(buf_size)) != std::floor(std::sqrt(buf_size))) // checking if perfect square...
        throw std::runtime_error("Invalid size in constructor, must be a perfect square");

    this->col_disp_count = 0;
    this->line_disp_count = 0;
    this->size = static_cast<int>(std::sqrt(buf_size));
    this->disp_limit = fact(this->size); // limit for disposition counters


    std::vector<uint8_t> tmp;
    for(int i = 0; i < buf_size; ++i) // copying initializer_list elements into the matrix
    {
        for(int j = 0; j < this->size; ++j)
        {
            tmp.push_back(buffer[i]);
            ++i;
        }
        --i;
        this->push_back(tmp);
        tmp.clear();
    }
}


DCMatrix::~DCMatrix()
{
}

/**
 * @brief method for getting the next disposition of the matrix
 * @details to get the next disposition of the matrix, we get the next disposition of each line, and the next columns disposition.
 * @return true if it is another next disposition avaible, false else.
 */
bool DCMatrix::next_disp() noexcept
{
    // switching lines dispositions
    for(auto it = this->begin(); it != this->end(); ++it)
        std::next_permutation(it->begin(), it->end());
    ++this->line_disp_count;
    
    // switching comlumns dispositions
    std::next_permutation(this->begin(), this->end(), [](const std::vector<uint8_t>&v1, const std::vector<uint8_t>&v2)
                                                        {
                                                               return (v1[0] < v2[0]);
                                                        });
    ++this->col_disp_count;    
    return this->line_disp_count != disp_limit and this->col_disp_count != disp_limit;
}

/**
 * @brief method for getting the previous disposition of the matrix
 * @details to get the previous disposition of the matrix, we get the previous disposition of each line, and the previous columns disposition.
 * @return true if it is another previous disposition avaible, false else.
 */
bool DCMatrix::prev_disp() noexcept
{
    // switching lines dispositions
    for(auto it = this->begin(); it != this->end(); ++it)
        std::prev_permutation(it->begin(), it->end());
    ++this->line_disp_count;

    // switching comlumns dispositions
    std::prev_permutation(this->begin(), this->end(), [](const std::vector<uint8_t>&v1, const std::vector<uint8_t>&v2)
                                                      {
                                                            return (v1[0] < v2[0]);
                                                      });
    ++this->col_disp_count;

    return this->line_disp_count != disp_limit and this->col_disp_count == disp_limit;
}

/**
 * @brief print the matrix to
 * 
 */
void DCMatrix::print() const noexcept
{
    for(auto it1 = this->begin(); it1 != this->end(); ++it1)
    {
        for(auto it2 = it1->begin(); it2 != it1->end(); ++it2)
            std::cout << (int)(*it2) << " " << "|";
        
        std::cout << "\n";
    }
}


/**
 * @brief overriding the std::vector::at() method
 * 
 * @param line line of the matrix
 * @param col col of the matrix
 * 
 * @return value at specified position
 * @exception std::runtime_error if the matrix bounds are violated
 */
uint8_t DCMatrix::at(int line, int col) const
{
    if (line < 0 or line >= this->size or col < 0 or col >= this->size)
        throw std::runtime_error("error invalid matrix input : line=" + std::to_string(line) + " col=" + std::to_string(col));

    return (*this)[line][col];
}   