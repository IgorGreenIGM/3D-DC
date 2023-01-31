#include <cmath>
#include <chrono>
#include <thread>
#include <cstring>

#include "../../include/Dcompress/DCQueue.h"


/**
 * @brief Construct a new DCQueue::DCQueue object 
 * 
 * @param buffer reference to the working-with buffer
 * @param queue_size size of the queue
 * @param matrix_size size of each matrix, as squared matrix, if input is n, the real value is n^2
 * @warning the buffer size **must** be a muliple to the square of the matrix size
 */
DCQueue::DCQueue(DCBuffer &buffer, int queue_size, int matrix_size) : buf_ref(buffer), queue_size(queue_size), matrix_size(matrix_size)
{
    this->counter = 0;
}


/**
 * @brief method for building matrix queue of specified size(got from constructor)
 * 
 * @param buf_delay delay in seconds for getting data from buffer
 * @param queue_delay delay in seconds for building wanted matrix
 * @note if the delay is passed, this method return the effective size of the queue build
 * 
 * @return queue_size if all okay or build matrix number if the delay is passed 
 */
int DCQueue::build(int buf_delay, int queue_delay)
{
    int rest_size = 0;
    uint8_t *rest_buffer = nullptr;
    uint8_t *all_data = nullptr;
    int matrix_len = this->matrix_size * this->matrix_size;

    auto sc = std::chrono::steady_clock();
    auto start = sc.now();

    while(this->counter != this->queue_size)
    {
        int data_size = 0;
        uint8_t *data_chunk = this->buf_ref.next_chunk(data_size, buf_delay);

        delete[] all_data;
        int all_data_size = rest_size + data_size;        
        all_data = new uint8_t[all_data_size];
        std::memcpy(all_data, rest_buffer, rest_size);
        std::memcpy(all_data + rest_size, data_chunk, data_size);

        int eff_build = 0;
        int can_build = std::floor((all_data_size) / matrix_len);
        
        eff_build = (this->counter + can_build <= this->queue_size) ? can_build : this->counter + can_build - this->queue_size;
        for(int i = 0; i < eff_build; ++i)
        {
            this->push_back({this->counter, DCMatrix(all_data + (i * matrix_len), matrix_len)});
            ++this->counter;
        }

        rest_size = all_data_size - (eff_build * matrix_len);

        delete[] rest_buffer;
        rest_buffer = new uint8_t[rest_size];
        std::memcpy(rest_buffer, all_data + all_data_size - rest_size, rest_size);

        if(static_cast<std::chrono::duration<double>>(sc.now() - start).count() > queue_delay) // if delay expired
            break;
    }

    if (rest_size != 0)
    {        
        int can_rest_build = std::floor(rest_size / matrix_len);
        for(int i = 0; i < can_rest_build; ++i)
        {
            this->push_back({this->counter, DCMatrix(rest_buffer + (i * matrix_len), matrix_len)});
            ++this->counter;
        }

        int f_rest_size = rest_size - can_rest_build * matrix_len;
        uint8_t *tmp_buf = new uint8_t[matrix_len];
        std::memset(tmp_buf, 0, matrix_len);
        std::memcpy(tmp_buf, rest_buffer + (can_rest_build * matrix_len), f_rest_size);

        ++this->counter;
        this->push_back({this->counter, DCMatrix(tmp_buf, matrix_len)});

        uint8_t *to_base256 = reinterpret_cast<uint8_t*>(&f_rest_size);
        std::memset(tmp_buf, 0, matrix_len);
        std::memcpy(tmp_buf, to_base256, 4);
        this->push_back({-1, DCMatrix(tmp_buf, matrix_len)});

        delete[] tmp_buf;
    }
    
    delete[] rest_buffer;
    delete[] all_data;

    return this->counter;
}


/**
 * @brief method for swapping 02 matrix from given ids
 * 
 * @param id_1 first matrix id
 * @param id_2 second matrix id
 * 
 * @exception std::runtime_error if one the inpur id's is note present
 */
void DCQueue::swap(int id_1, int id_2)
{
    if(id_1 > this->counter || id_1 < 0)
        throw std::runtime_error("error when getting the matrix of id : " + std::to_string(id_1));
    
    if(id_2 > this->counter || id_2 < 0)
        throw std::runtime_error("error when getting the matrix of id : " + std::to_string(id_2));

    // serching for pairs with given id's
    auto it_1 = this->begin();
    auto it_2 = this->begin();
    for(auto it = this->begin(); it != this->end(); ++it)
    {
        if(it->first == id_1)
            it_1 = it;
        
        if(it->first == id_2)
            it_2 = it;
    }

    // swaping
    std::iter_swap(it_1, it_2);
}