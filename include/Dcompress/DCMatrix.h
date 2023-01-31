#ifndef _DC_MATRIX_H_INCLUDED_
#define _DC_MATRIX_H_INCLUDED_

#include <vector>
#include <iostream>


class DCMatrix : private std::vector<std::vector<uint8_t>>
{
    public :
        DCMatrix(std::initializer_list<uint8_t> &list);
        DCMatrix(std::vector<std::vector<uint8_t>> &matrix);
        DCMatrix(uint8_t *buffer, int buf_size);
        DCMatrix();
        ~DCMatrix();

        bool next_disp();
        bool prev_disp();
        void print();

    private :
        int size; // the size is the number of elements in each line and each colum, then the total is (size*size)
        int col_disp_count; // actual column disposition counter (default is 0). 
        int line_disp_count; // actual lines disposition counter (default is 0).
        int disp_limit; // disposition limit count, typically (n!) for lines and (n!) for columns. 
};


constexpr unsigned long long int fact(int n)
{
    return (n == 0) ? 1 : n * fact(n - 1);
}
#endif //_DC_MATRIX_H_INCLUDED_