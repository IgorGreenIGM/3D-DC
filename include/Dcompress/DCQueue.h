#ifndef _DC_QUEUE_H_INCLUDED_
#define _DC_QUEUE_H_INCLUDED_

#include <map>
#include <vector>

#include "./DCMatrix.h"
#include "./DCBuffer.h"

class DCQueue : public std::vector<std::pair<int, DCMatrix>>
{
    public : 
        DCQueue(DCBuffer &buffer, int queue_size, int matrix_size);

        int build(double buf_delay, double queue_delay);
        void swap(int id_1, int id_2);

    private :
        int counter; // counter of builded matrix
        int queue_size; // size of the queue
        int matrix_size; // size of the matrix

        DCBuffer &buf_ref; // reference of the working-with buffer
};

#endif // _DC_QUEUE_H_INCLUDED_