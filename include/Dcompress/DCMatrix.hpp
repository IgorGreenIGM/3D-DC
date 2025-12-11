#ifndef _DC_MATRIX_H_INCLUDED_
#define _DC_MATRIX_H_INCLUDED_

#include <vector>
#include <iostream>
#include <unordered_map>

class DCMatrix : private std::vector<std::vector<uint8_t>>
{
    public :
        explicit DCMatrix(const std::initializer_list<uint8_t> &list);
        explicit DCMatrix(const std::vector<std::vector<uint8_t>> &matrix);
        explicit DCMatrix(uint8_t value, int size);
        DCMatrix(uint8_t *buffer, int buf_size);

        void print() const noexcept;
        bool next_disp() noexcept;
        bool prev_disp() noexcept;

        const uint8_t &operator()(std::size_t line, std::size_t col) const 
        { 
            return (*this)[line][col];
        }
        
        uint8_t at(std::size_t line, std::size_t col) const;
        uint8_t max() const noexcept;

        std::vector<uint8_t> get_linear() const noexcept;

        friend class DCQueue;

        inline std::size_t get_size() const noexcept {return this->size;}

        std::unordered_map<uint8_t, double> prob_distribution() const noexcept;

    private :
        std::size_t size; // the size is the number of elements in each line and each colum, then the total is (size*size)
        std::size_t col_disp_count; // actual column disposition counter (default is 0). 
        std::size_t line_disp_count; // actual lines disposition counter (default is 0).
        std::size_t disp_limit; // disposition limit count, typically (n!) for lines and (n!) for columns. 

};


constexpr unsigned long long int fact(int n)
{
    return (n == 0) ? 1 : n * fact(n - 1);
}
#endif //_DC_MATRIX_H_INCLUDED_