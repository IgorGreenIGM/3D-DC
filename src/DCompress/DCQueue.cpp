#include <cmath>
#include <chrono>
#include <thread>
#include <future>
#include <cstring>
#include <algorithm>
#include <bits/unique_ptr.h>

#include "../../include/Dcompress/DCQueue.hpp"

/**
 * @brief Construct a new DCQueue::DCQueue object 
 * 
 * @param buffer reference to the working-with buffer
 * @param queue_size size of the queue
 * @param matrix_size size of each matrix, as squared matrix, if input is n, the real value is n^2
 * @warning the buffer size **must** be a muliple to the square of the matrix size
 * 
 * @exception std::runtime_error if the queue_size or matrix size == 0
 */
DCQueue::DCQueue(DCBuffer &buffer, int queue_size, int matrix_size) : buf_ref(buffer), queue_size(queue_size), matrix_size(matrix_size), counter(0)
{
    if (queue_size <= 0)
        throw std::runtime_error("error cannot construct DCQueue with size = " + std::to_string(queue_size));
    if (matrix_size <= 0 or matrix_size == 1)
        throw std::runtime_error("error, cannot construct DCQUeue matrix size = " + std::to_string(matrix_size));
}       


/**
 * @brief method for building matrix queue of specified size
 * 
 * @param buf_delay delay until the buffer will force unlocking(unit = sec)
 * @param queue_delay delay between two pass when building an unique matrix inside the Queue(unit = sec)
 * @see DCBuffer::next_chunk()
 * @note if the delay is passed, this method return the effective size of the queue build
 * @warning After building matrix(in cases the the wanted number is effective or not),
 * @warning method will build matrix with rest of datas(one or many, depending of the rest of data size) 
 * @warning in the buffer in zero completion(meaning that if the data is insufficient to build , the will be completed to 0).
 * @warning -
 * @warning Then to store the size of the data written in the last matrix, the method will build a last matrix called end_matrix with id = -1.
 * @warning end_matrix has  the same size as others, it contain the length of the data written in the last matrix.
 * @warning As the DCMatrix values are uint8_t(1 byte), we convert this length in base 256 and write it into the end_matrix in zero completion.
 * 
 * @return std::size_t queue_size if all okay or build matrix number if the delay is passed 
 */
std::size_t DCQueue::build(double buf_delay, double queue_delay) noexcept
{
    // starting by resenting the Queue
    this->clear();
    this->counter = 0;

    // setting up the rest buffer, all_data will contain the concatenation
    //  of the rest buffer and the new read data
    auto rest_buffer = std::unique_ptr<uint8_t[]>(nullptr);
    auto all_data = std::unique_ptr<uint8_t[]>(nullptr);
    int rest_size = 0; 
    int matrix_len = this->matrix_size * this->matrix_size;
    
    // setting up timer
    auto sc = std::chrono::steady_clock();
    auto start = sc.now();

    while(this->counter != this->queue_size) // until wanted number of matrix are not build...
    {   
        // reading data from buffer
        int data_size = 0;
        const std::unique_ptr<uint8_t[]> data_chunk(this->buf_ref.next_chunk(data_size, buf_delay)); 

        //  copying data from buffer 
        int all_data_size = rest_size + data_size;        
        all_data.reset(new uint8_t[all_data_size]);
        std::memcpy(all_data.get(), rest_buffer.get(), rest_size);
        std::memcpy(all_data.get() + rest_size, data_chunk.get(), data_size);

        int can_build = std::floor(all_data_size / matrix_len);  // number of matrix that can be build from all_data length
        int eff_build = (this->counter + can_build <= this->queue_size) ? can_build : this->queue_size - this->counter; // number of matrix that can be build without overcome the Queue size
        for(int i = 0; i < eff_build; ++i)
        {
            this->emplace_back(this->counter, DCMatrix(all_data.get() + (i * matrix_len), matrix_len));// building and storing...
            ++this->counter;
        }

        // copying the rest of data in rest buffer
        rest_size = all_data_size - (eff_build * matrix_len);
        rest_buffer.reset(new uint8_t[rest_size]);
        std::memcpy(rest_buffer.get(), all_data.get() + all_data_size - rest_size, rest_size);

        if(static_cast<std::chrono::duration<double>>(sc.now() - start).count() > queue_delay) // if delay expired, 
            break;
        else
            start = sc.now(); // reset the delay        
    }

    if (rest_size != 0) 
    {       
        int can_rest_build = std::floor(rest_size / matrix_len); // number of matrix can be build from rest_buffer
        for(int i = 0; i < can_rest_build; ++i)
        {
            this->emplace_back(this->counter, DCMatrix(rest_buffer.get() + (i * matrix_len), matrix_len));
            ++this->counter;
        }

        /*building matrix with rest of data*/
        int f_rest_size = rest_size - (can_rest_build * matrix_len); // final rest size
        uint8_t *tmp_buf = new uint8_t[matrix_len];
        if (f_rest_size != 0)
        {
            std::memset(tmp_buf, 0, matrix_len);
            std::memcpy(tmp_buf, rest_buffer.get() + (can_rest_build * matrix_len), f_rest_size);

            this->emplace_back(this->counter, DCMatrix(tmp_buf, matrix_len));
            ++this->counter;
        }

        /*building end_matrix*/
                    
        // bulding...
        std::memset(tmp_buf, 0, matrix_len);  
        uint8_t *to_base256 = reinterpret_cast<uint8_t*>(&f_rest_size);

        if(is_big_endian())
            std::memcpy(tmp_buf, to_base256, 4);
        else
            for(int i = 0; i < sizeof(int); ++i)
                tmp_buf[i] = to_base256[3 - i];
        
        this->emplace_back(-1, DCMatrix(tmp_buf, matrix_len));
        delete[] tmp_buf;
    }
    else // until there's no rest, we always build the end_matrix to zero completion
    {
        uint8_t *tmp_buf = new uint8_t[matrix_len];
        std::memset(tmp_buf, 0, matrix_len);
        this->emplace_back(-1, DCMatrix(tmp_buf, matrix_len));
        delete[] tmp_buf;
    }

    return this->counter;
}


/**
 * @brief method for swapping 02 matrix from given ids
 * @param id_1 first matrix id
 * @param id_2 second matrix id
 * @exception std::runtime_error if one the input id's is note present
 */
void DCQueue::swap(int id_1, int id_2)
{
    if(id_1 > this->counter or id_1 < 0)
        throw std::runtime_error("error when getting the matrix of id : " + std::to_string(id_1));
    
    if(id_2 > this->counter or id_2 < 0)
        throw std::runtime_error("error when getting the matrix of id : " + std::to_string(id_2));

    // serching for pairs with given id's
    auto it_1 = this->begin(), it_2 = this->begin();
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

/**
 * @brief get all cells from the matrix in specific position
 * @return std::vector<uint8_t>
 */
std::vector<uint8_t> DCQueue::get_z(int line, int col) const noexcept
{
    std::vector<uint8_t> out;
    for(auto it = this->begin(); it != this->end() - 1; ++it) // excluding end_matrix
        out.push_back(it->second[line][col]);
    
    return out;
}

/**
 * @brief get all cells in the Queue by z_axis
 * @return std::vector<uint8_t> 
 */
std::vector<uint8_t> DCQueue::get_all_z() const noexcept
{
    std::vector<uint8_t> out;
    for (int line  = 0; line < this->matrix_size; ++line)
        for(int col = 0; col < this->matrix_size; ++col)
            for (auto it = this->begin(); it != this->end() - 1; ++it) // ecluding end_matrix
                out.push_back(it->second[line][col]);
    
    return out;
}

/**
 * @brief get entropy of the whole Queue, excluding end_matrix and zero-completion values
 * 
 * @return std::size_t 
 */
double DCQueue::entropy() const noexcept
{
    std::map<uint8_t, int> frequency;

    // each line of each matrix excluding end matrix and last matrix
    for (auto it = this->begin(); it != this->end() - 2; ++it) 
        for (int line  = 0; line < this->matrix_size; ++line)
            for(int col = 0; col < this->matrix_size; ++col)
                ++frequency[it->second[line][col]];

    auto last_count = this->last_data_size();

    // each line of last matrix
    for(int i = 0; i < std::floor(last_count / this->matrix_size); ++i)
        for(int j = 0; j < this->matrix_size; ++j)
            ++frequency[(this->end() - 2)->second[i][j]];

    // last line
    for(int i = 0; i < last_count - (std::floor(last_count / this->matrix_size) * this->matrix_size); ++i)
        ++frequency[(this->end() - 2)->second[std::floor(last_count / this->matrix_size)][i]];

    // compute
    double _entropy = .0;
    double length = static_cast<double>((this->counter - 1) * this->matrix_size * this->matrix_size + last_count); // excluding zero-completion values
    std::cout << "computed : " << (int)length << "\n";
    for(const auto &freq : frequency)
    {
        double p_x = static_cast<double>(freq.second) / length;
        _entropy -= p_x * std::log2f(p_x);  
    }

    return _entropy;
}


/**
 * @brief get all the elements inside the queue, excluding end matrix
 * @param filtred if the Queue is filtred at the moment of the call
 * @param unfiltred if the Queue is unfiltred at the moment of the call 
 * 
 * @return std::vector<uint8_t> 
 */
std::vector<uint8_t> DCQueue::get_linear(bool filtred, bool unfiltred) const noexcept
{
    std::vector<uint8_t> out; // output

    if(filtred)
    {
        // getting data, excluding end_matrix
        for(auto it = this->begin(); it != this->end() - 1; ++it)
            for(int i = 0; i < matrix_size; ++i)
                for(int j = 0; j < matrix_size; ++j)
                    out.push_back(it->second[i][j]);
    }
    else if (not unfiltred) // meaning that Queue is not unfiltred and not filtred, can be used to check if build method works!
    {
        // getting data, excluding end_matrix and last matrix
        for(auto it = this->begin(); it != this->end() - 2; ++it)
            for(int i = 0; i < matrix_size; ++i)
                for(int j = 0; j < matrix_size; ++j)
                    out.push_back(it->second[i][j]);
        
        // from end_matrix, computing the size of data written in the last matrix
        int last_count = this->last_data_size();

        // getting last datas on each line of last matrix
        for(int i = 0; i < std::floor(last_count / this->matrix_size); ++i)
            for(int j = 0; j < this->matrix_size; ++j)
                out.push_back((this->end() - 2)->second[i][j]);

        // getting last datas
        for(int i = 0; i < last_count - (std::floor(last_count / this->matrix_size) * this->matrix_size); ++i)
            out.push_back((this->end() - 2)->second[std::floor(last_count / this->matrix_size)][i]);
    }
    else // meaning that Queue is unfiltred
    {
        // getting data, excluding the end_matrix of the actual Queue, the unfiltred end_matrix, and the unfiltred last matrix  
        for(auto it = this->begin(); it != this->end() - 3; ++it)
            for(int i = 0; i < matrix_size; ++i)
                for(int j = 0; j < matrix_size; ++j)
                    out.push_back(it->second[i][j]);

        // from unfiltred end_matrix, computing the size of data written in the last matrix
        int last_count {0};
        for (int i = 0; i < sizeof(int); ++i)
            last_count += (this->end() - 2)->second[0][i] * std::pow(256, 3 - i);

        if (last_count == 0) // meaning that the data size in the unfiltred last matrix is 0, we simple copy it
        {
            for(int i = 0; i < matrix_size; ++i)
                for(int j = 0; j < matrix_size; ++j)
                    out.push_back((this->end() - 3)->second[i][j]);
        }
        else
        {
            // getting last datas on each line of last matrix
            for(int i = 0; i < std::floor(last_count / this->matrix_size); ++i)
                for(int j = 0; j < this->matrix_size; ++j)
                    out.push_back((this->end() - 3)->second[i][j]);

            // getting last datas
            for(int i = 0; i < last_count - (std::floor(last_count / this->matrix_size) * this->matrix_size); ++i)
                out.push_back((this->end() - 3)->second[std::floor(last_count / this->matrix_size)][i]);   
        }
    }
    return out;
}

/**
 * @brief method for getting all data form the Queue, including end_matrix
 * 
 * @return std::vector<uint8_t> 
 */
std::vector<uint8_t> DCQueue::get_all_linear() const noexcept
{
    std::vector<uint8_t> out; // output

    for(auto it = this->begin(); it != this->end(); ++it)
        for(int i = 0; i < matrix_size; ++i)
            for(int j = 0; j < matrix_size; ++j)
                out.push_back(it->second[i][j]);

    return out;
}


/**
 * @brief from end_matrix, return the size of data written in the last matrix ignoring zero compression values
 * 
 * @return std::size_t 
 */
std::size_t DCQueue::last_data_size() const noexcept
{
    std::size_t last_count(0);

    auto end_content ((this->end() - 1)->second.get_linear()); // last matrix
    for (int i = 0; i < end_content.size() and i < sizeof(int); ++i)
        last_count += end_content[i] * std::pow(256, sizeof(int) - 1 - i); // no need to check endianess here, refers to DCQueue::build()

    return last_count;
}


/**
 * @brief method for filtering the DCQueue
 * @details it consist to apply several filters on each matrix in the queue, excluding end_matrix
 * @details there are 02 types of filters : 2D filters and 3D filters. 
 * @details 2D filters are applied on each single matrix in the Queue. avaible 2D filters are : NO, SUB, UP, AVERAGE and PAETH
 * @details 3D filters are applied on each specific cell in each matrix of the Queue. avaible 3D filters are : DC_NO, DC_SUB, DC_UP, DC_AVERAGE and DC_PAETH
 * @details the method apply each filter, and return a dictionnary containing the filters id that maximise datas redundancy(minimize entropy).
 * @details the method start by 2D filters after 3D. the the dictionnary will contain first 2D filters ids and the 3D, up to 256 filters are avaible.
 * @details method now supports multithreading ! 
 * @see enum FILTER_MODE
 * @param _2D perform 2D filters
 * @param _3D perform 3D filters
 * 
 * @return std::vector<uint8_t> the filter dictionnary
 */
std::vector<uint8_t> DCQueue::filter(bool _2D, bool _3D) noexcept
{ 
    // 2d filter lambda function, return filtered line 
    auto filter_line2D = [this](int filter_mode, DCQueue::iterator queue_it, const std::vector<uint8_t> &no_fltr_prev_line, int line) -> std::vector<uint8_t>
    {
        std::vector<uint8_t> filtred_line(this->matrix_size);
        
        switch (filter_mode)
        {
            case FILTER_MODE::NO_2D :
                std::copy_n(queue_it->second[line].begin(), this->matrix_size, filtred_line.begin());
            break;

            case FILTER_MODE::SUB_2D :
                filtred_line[0] = queue_it->second[line][0];
                for (int j = 1; j < this->matrix_size; ++j)
                    filtred_line[j] = queue_it->second[line][j] - queue_it->second[line][j - 1];
            break;

            case FILTER_MODE::UP_2D : 
                if (line == 0)
                    std::copy_n(queue_it->second[line].begin(), this->matrix_size, filtred_line.begin());
                else
                    for (int j = 0; j < this->matrix_size; ++j)
                        filtred_line[j] = queue_it->second[line][j] - no_fltr_prev_line[j];
            break;

            case FILTER_MODE::AVERAGE_2D : 
                if(line == 0)
                {
                    filtred_line[0] = queue_it->second[line][0];
                    for (int j = 1; j < this->matrix_size; ++j)
                        filtred_line[j] = queue_it->second[line][j] - std::floor(queue_it->second[line][j - 1] / 2);                
                }
                else
                {
                    filtred_line[0] = queue_it->second[line][0] - std::floor(no_fltr_prev_line[0] / 2);

                    for(int j = 1; j < this->matrix_size; ++j)
                        filtred_line[j] = queue_it->second[line][j] - std::floor((queue_it->second[line][j - 1] + no_fltr_prev_line[j]) / 2);
                }
            break;

            case FILTER_MODE::PAETH_2D : 
                if (line == 0)
                {
                    filtred_line[0] = queue_it->second[line][0];
                    for (int j = 1; j < this->matrix_size; ++j)
                        filtred_line[j] = queue_it->second[line][j] - paeth_pred(queue_it->second[line][j - 1], 0, 0);                    
                }
                else
                {
                    filtred_line[0] = queue_it->second[line][0] - no_fltr_prev_line[0];

                    for (int j = 1; j < this->matrix_size; ++j)
                        filtred_line[j] = queue_it->second[line][j] - paeth_pred(queue_it->second[line][j - 1], no_fltr_prev_line[j], no_fltr_prev_line[j - 1]);
                }
            break;

            default :
                throw std::runtime_error("invalid 2D filter specified : " + std::to_string(filter_mode));
            break;
        }

        return filtred_line;
    };

    // 3d filter lambda function, return filtred line 
    auto filter_line3D = [this](int filter_mode, const std::vector<uint8_t> &no_fltr_prev_line, int line, int col) -> std::vector<uint8_t>
    {
        std::vector<uint8_t> filtred_line(this->counter);
        auto z_line(this->get_z(line, col)); 
        
        switch (filter_mode)
        {                        
            case FILTER_MODE::NO_3D : 
                std::copy_n(z_line.begin(), z_line.size(), filtred_line.begin());
            break;

            case FILTER_MODE::SUB_3D :
                filtred_line[0] = z_line[0];
                for (int j = 1; j < z_line.size(); ++j)
                    filtred_line[j] = z_line[j] - z_line[j - 1];
            break;

            case FILTER_MODE::UP_3D :
                if (line == 0)
                    std::copy_n(z_line.begin(), z_line.size(), filtred_line.begin());
                else
                    for (int j = 0; j < z_line.size(); ++j)
                        filtred_line[j] = z_line[j] - no_fltr_prev_line[j];
            break;

            case FILTER_MODE::AVERAGE_3D : 
                if(line == 0)
                {           
                    filtred_line[0] = z_line[0];
                    for (int j = 1; j < z_line.size(); ++j)
                        filtred_line[j] = z_line[j] - std::floor(z_line[j - 1] / 2);                
                }
                else
                {
                    filtred_line[0] = z_line[0] - std::floor(no_fltr_prev_line[0] / 2);
                    for(int j = 1; j < z_line.size(); ++j)
                        filtred_line[j] = z_line[j] - std::floor((z_line[j - 1] + no_fltr_prev_line[j]) / 2);
                }
            break;

            case FILTER_MODE::PAETH_3D :
                if (line == 0)
                {
                    filtred_line[0] = z_line[0];
                    for (int j = 1; j < z_line.size(); ++j)
                        filtred_line[j] = z_line[j] - paeth_pred(z_line[j - 1], 0, 0);                    
                }
                else
                {
                    filtred_line[0] = z_line[0] - no_fltr_prev_line[0];

                    for (int j = 1; j < z_line.size(); ++j)
                        filtred_line[j] = z_line[j] - paeth_pred(z_line[j - 1], no_fltr_prev_line[j], no_fltr_prev_line[j - 1]);
                }
            break;

            default :
                std::runtime_error("invalid 3D filter specified : " + std::to_string(filter_mode));
            break;
        }

        return filtred_line;
    };

    // **dictionnary output** 
    std::vector<uint8_t> filter_dict; 
    
    if (_2D) // 2D perform
    {
        // lambda for parallelise 2D filtering
        auto _2D_thread = [&filter_line2D, this](DCQueue::iterator start_it, DCQueue::iterator end_it, std::promise<std::pair<std::thread::id, std::vector<uint8_t>>> &prom) -> void
        {
            // thread local dictionnary 
            std::vector<uint8_t> local_dict;

            /* we'll perfrom 2D filtering on each line of each matrix and store the filter mode of the one with minimal entropy */
            std::vector<uint8_t> no_fltr_prev_line;
            std::vector<std::vector<uint8_t>> filtred_lines;  
            for (auto queue_it = start_it; queue_it != end_it; ++queue_it)
            {
                for (int line = 0; line < this->matrix_size; ++line) 
                {
                    filtred_lines.emplace_back(filter_line2D(FILTER_MODE::NO_2D, queue_it, {}, line));
                    filtred_lines.emplace_back(filter_line2D(FILTER_MODE::SUB_2D, queue_it, {}, line));
                    filtred_lines.emplace_back(filter_line2D(FILTER_MODE::UP_2D, queue_it, no_fltr_prev_line, line));
                    filtred_lines.emplace_back(filter_line2D(FILTER_MODE::AVERAGE_2D, queue_it, no_fltr_prev_line, line));
                    filtred_lines.emplace_back(filter_line2D(FILTER_MODE::PAETH_2D, queue_it, no_fltr_prev_line, line));

                    auto min = std::min_element(filtred_lines.begin(), filtred_lines.end(), [](const std::vector<uint8_t> &v1, const std::vector<uint8_t> &v2)
                                                                                            {
                                                                                                return _entropy(v1) < _entropy(v2);
                                                                                            });
                    
                    local_dict.push_back(FILTER_MODE::NO_2D + std::distance(filtred_lines.begin(), min));
                    
                    // copying actual line(not yet filtered) of actual matrix as the next "not filtred previous"
                    std::swap(no_fltr_prev_line, queue_it->second[line]);
                    // writing filtered line into matrix
                    std::swap(queue_it->second[line], *min);

                    filtred_lines.clear();
                }
            }
            // giving the result to promise
            prom.set_value(std::make_pair(std::this_thread::get_id(), std::move(local_dict)));
        };

        // get CPU logical UC availbles and working range size
        const unsigned int nb_threads = std::thread::hardware_concurrency();
        const float range_size = static_cast<float>(this->counter) / nb_threads;
        
        if (range_size < 1) // starts single thread
        {
            std::promise<std::pair<std::thread::id, std::vector<uint8_t>>> prom; // promise            
            std::thread th(_2D_thread, this->begin(), this->end() - 1, std::ref(prom)); // thread create...
            th.join(); // waiting thread to finish 
            auto result(prom.get_future().get()); // storing result
            filter_dict.swap(result.second); // copying to filter dictionnary 
        }
        else
        {
            int th_inc = 0;
            std::vector<std::thread> threads;
            std::vector<std::pair<std::thread::id, std::vector<uint8_t>>> results;
            std::vector<std::promise<std::pair<std::thread::id, std::vector<uint8_t>>>> promises(nb_threads);

            // create threads
            for (th_inc = 1; th_inc < nb_threads; ++th_inc)
                threads.emplace_back(_2D_thread, this->begin() + (th_inc - 1) * range_size, this->begin() + th_inc * range_size, std::ref(promises[th_inc - 1]));
            threads.emplace_back(_2D_thread, this->begin() + (nb_threads - 1) * range_size, this->end() - 1, std::ref(promises[nb_threads - 1])); // this->end() - 1 for exclude end_matrix

            for (th_inc = 0; th_inc < nb_threads; ++th_inc)
                threads[th_inc].join();

            for (th_inc = 0; th_inc < nb_threads; ++th_inc)
                results.emplace_back(promises[th_inc].get_future().get());

            /*to respect order of filtering when copying threads-local dictionnaries to output, we should sort the results by thread ids*/
            std::sort(results.begin(), results.end(), [](const std::pair<std::thread::id, std::vector<uint8_t>> &p1, const std::pair<std::thread::id, std::vector<uint8_t>> &p2)
                                                      {
                                                            return p1.first < p2.first;
                                                      });

            // copying threads-local dictionnaries to output
            for (th_inc = 0; th_inc < nb_threads; ++th_inc)
                std::move(results[th_inc].second.begin(), results[th_inc].second.end(), std::back_inserter(filter_dict));
        }
    }

    if (_3D) // perform 3D
    {
        /* we'll now perfrom 2D filtering on each "z-line" and store the filter mode of the one with minimal entropy */
        std::vector<std::vector<uint8_t>> filtred_lines; 
        std::vector<uint8_t> no_fltr_prev_line(this->counter);

        for (int line = 0; line < this->matrix_size; ++line)
        {
            for (int col = 0; col < this->matrix_size; ++col)
            {
                filtred_lines.emplace_back(filter_line3D(FILTER_MODE::NO_3D, {}, line, col));
                filtred_lines.emplace_back(filter_line3D(FILTER_MODE::SUB_3D, {}, line, col));
                filtred_lines.emplace_back(filter_line3D(FILTER_MODE::UP_3D, no_fltr_prev_line, line, col));
                filtred_lines.emplace_back(filter_line3D(FILTER_MODE::AVERAGE_3D, no_fltr_prev_line, line, col));
                filtred_lines.emplace_back(filter_line3D(FILTER_MODE::PAETH_3D, no_fltr_prev_line, line, col));

                auto min = std::min_element(filtred_lines.begin(), filtred_lines.end(), [](const std::vector<uint8_t> &v1, const std::vector<uint8_t> &v2)
                                                                                        {
                                                                                            return _entropy(v1) < _entropy(v2);
                                                                                        });

                filter_dict.push_back(FILTER_MODE::NO_3D + std::distance(filtred_lines.begin(), min));

                int line_inc = 0;                
                for(auto queue_it = this->begin(); queue_it != this->end() - 1; ++queue_it, ++line_inc)
                {
                    // copying actual "z_line"(not yet filtered) of actual matrix as the next "not filtred previous"
                    no_fltr_prev_line[line_inc] = queue_it->second[line][col];
                    
                    // writing filtered line into "z_line"
                    queue_it->second[line][col] = (*min)[line_inc];
                }

                filtred_lines.clear();
            }
        }
    }

    return filter_dict;
}


/**
 * @brief method for unfiltering the DCQueue using filter dictionnary 
 * 
 * @param filter_dict filter dictionnary 
 * @param _2D perform 2D unfilter
 * @param _3D perform 3D unfilter
 * @exception std::runtime_error if invalid Filter id specified
 */
void DCQueue::unfilter(const std::vector<uint8_t> &filter_dict, bool _2D, bool _3D)
{
    // perform 2D unfilter lambda 
    auto unfilter_line2D = [this](int filter_mode, std::vector<std::pair<int, DCMatrix>>::iterator queue_it, int line, const std::vector<uint8_t> &un_fltrd_prev_line) -> std::vector<uint8_t>
    {
        std::vector<uint8_t> unfiltred_line(this->matrix_size);
        switch (filter_mode)
        {
            case FILTER_MODE::NO_2D :
                std::copy_n(queue_it->second[line].begin(), this->matrix_size, unfiltred_line.begin());
            break;

            case FILTER_MODE::SUB_2D :
                unfiltred_line[0] = queue_it->second[line][0];
                for (int j = 1; j < this->matrix_size; ++j)
                    unfiltred_line[j] = queue_it->second[line][j] + unfiltred_line[j - 1];
            break;

            case FILTER_MODE::UP_2D : 
                if (line == 0)
                    std::copy_n(queue_it->second[line].begin(), this->matrix_size, unfiltred_line.begin());
                else
                    for (int j = 0; j < this->matrix_size; ++j)
                        unfiltred_line[j] = queue_it->second[line][j] + un_fltrd_prev_line[j];
            break;

            case FILTER_MODE::AVERAGE_2D : 
                if(line == 0)
                {
                    unfiltred_line[0] = queue_it->second[line][0];
                    for (int j = 1; j < this->matrix_size; ++j)
                        unfiltred_line[j] = queue_it->second[line][j] + std::floor(unfiltred_line[j - 1] / 2);                
                }
                else
                {
                    unfiltred_line[0] = queue_it->second[line][0] + std::floor(un_fltrd_prev_line[0] / 2);
                    for(int j = 1; j < this->matrix_size; ++j)
                        unfiltred_line[j] = queue_it->second[line][j] + std::floor((unfiltred_line[j - 1] + un_fltrd_prev_line[j]) / 2);
                }
            break;

            case FILTER_MODE::PAETH_2D : 
                if (line == 0)
                {
                    unfiltred_line[0] = queue_it->second[line][0];
                    for (int j = 1; j < this->matrix_size; ++j)
                        unfiltred_line[j] = queue_it->second[line][j] + paeth_pred(unfiltred_line[j - 1], 0, 0);                    
                }
                else
                {
                    unfiltred_line[0] = queue_it->second[line][0] + un_fltrd_prev_line[0];
                    for (int j = 1; j < this->matrix_size; ++j)
                        unfiltred_line[j] = queue_it->second[line][j] + paeth_pred(unfiltred_line[j - 1], un_fltrd_prev_line[j], un_fltrd_prev_line[j - 1]);
                }
            break;

            default :
                throw std::runtime_error("invalid 2D filter specified : " + std::to_string(filter_mode));
            break;
        }

        return unfiltred_line;
    };

    // lambda to perform 3D unfitering, no return, a reference must be passed in with the output will be copied
    auto unfilter_line3D = [this](int filter_mode, int line, int col, const std::vector<uint8_t> &un_fltrd_prev_line) -> std::vector<uint8_t>
    {
        const auto z_line(std::move(this->get_z(line, col))); // all deep cells
        std::vector<uint8_t> unfiltred_line(z_line.size());
        
        switch (filter_mode)
        {                        
            case FILTER_MODE::NO_3D : 
                std::copy_n(z_line.begin(), z_line.size(), unfiltred_line.begin());
            break;

            case FILTER_MODE::SUB_3D :
                unfiltred_line[0] = z_line[0];
                for (int j = 1; j < z_line.size(); ++j)
                    unfiltred_line[j] = z_line[j] + unfiltred_line[j - 1];
            break;

            case FILTER_MODE::UP_3D :
                if (line == 0)
                    std::copy_n(z_line.begin(), z_line.size(), unfiltred_line.begin());
                else
                    for (int j = 0; j < z_line.size(); ++j)
                        unfiltred_line[j] = z_line[j] + un_fltrd_prev_line[j];
            break;

            case FILTER_MODE::AVERAGE_3D : 
                if(line == 0)
                {
                    unfiltred_line[0] = z_line[0];
                    for (int j = 1; j < z_line.size(); ++j)
                        unfiltred_line[j] = z_line[j] + std::floor(unfiltred_line[j - 1] / 2); 
                }
                else
                {
                    unfiltred_line[0] = z_line[0] + std::floor(un_fltrd_prev_line[0] / 2);
                    for(int j = 1; j < z_line.size(); ++j)
                        unfiltred_line[j] = z_line[j] + std::floor((unfiltred_line[j - 1] + un_fltrd_prev_line[j]) / 2);
                }
            break;

            case FILTER_MODE::PAETH_3D :
                if (line == 0)
                {
                    unfiltred_line[0] = z_line[0];
                    for (int j = 1; j < z_line.size(); ++j)
                        unfiltred_line[j] = z_line[j] + paeth_pred(unfiltred_line[j - 1], 0, 0);                    
                }
                else
                {
                    unfiltred_line[0] = z_line[0] + un_fltrd_prev_line[0];
                    for (int j = 1; j < z_line.size(); ++j)
                        unfiltred_line[j] = z_line[j] + paeth_pred(unfiltred_line[j - 1], un_fltrd_prev_line[j], un_fltrd_prev_line[j - 1]);
                }
            break;

            default :
                std::runtime_error("invalid 3D filter specified : " + std::to_string(filter_mode));
            break;
        }

        return unfiltred_line;
    };

    /*filter_inc indicates where 2D or 3D filters id starts in the filter dictionnary.
    when filtering, if there 2D and 3D filters was performed, the order is 2D then 3D.
    so to perform unfilter, we need(mathematically) to start by 3D filters. */
    unsigned int filter_inc = 0;

    if (_3D) // perform 3D
    {
        filter_inc = (not _2D) ? 0 : this->matrix_size * (this->counter);

        std::vector<uint8_t> un_fltrd_prev_line(this->counter); 
        for (int line = 0; line < this->matrix_size; ++line)
        {
            for (int col = 0; col < this->matrix_size; ++col)
            {
                // unfilter...
                std::vector<uint8_t> unfiltred_line(unfilter_line3D(filter_dict[filter_inc], line, col, un_fltrd_prev_line));

                // writing unfiltered cells in queue, excluding the end_matrix
                int i = 0;
                for(auto it = this->begin(); it != this->end() - 1; ++it, ++i)
                    it->second[line][col] = unfiltred_line[i];
                // copying actual line(already unfiltered) of actual matrix as the next "unfiltred previous"
                std::swap(unfiltred_line, un_fltrd_prev_line);

                ++filter_inc;
            }
        }
    }
    
    if (_2D)
    {
        filter_inc = 0;
        
        std::vector<uint8_t> un_fltrd_prev_line(this->matrix_size);
        for(auto queue_it = this->begin(); queue_it != this->end() - 1; ++queue_it) // excluding end_matrix 
        {
            for(int line = 0; line < this->matrix_size; ++line)
            {
                // unfilter...
                std::vector<uint8_t> unfiltred_line(unfilter_line2D(filter_dict[filter_inc], queue_it, line, un_fltrd_prev_line)); 
                
                // copying actual line(already filtered) of actual matrix as the next "un_filtred previous" 
                std::copy(unfiltred_line.begin(), unfiltred_line.end(), un_fltrd_prev_line.begin());
                // writing filtered line into matrix
                std::copy(unfiltred_line.begin(), unfiltred_line.end(), queue_it->second[line].begin());

                ++filter_inc;
            }
        }
    }
}


/**
 * @brief Method for filter and save an input file.
 * 
 * @param file_name input filer name
 * @param ids_file_out file name for filters ids output
 * @param filtred_file_out file name for filtred output 
 * @param matrix_size size of each matrix inside the Queue
 * @param buffer_size size of the buffer 
 * @param queue_size size of the Queue
 * @param _2D if 2D filter
 * @param _3D if 3D filter
 * @param buf_delay timer for getting data from buffer
 * @param queue_delay timer for building 
 * 
 * @exception std::runtime_error if error when creating output files
 */
void DCQueue::filter_file(const std::string &file_name, const std::string &ids_file_out, const std::string &filtred_file_out, int matrix_size, int buffer_size, int queue_size, bool _2D, bool _3D, double buf_delay, double queue_delay)
{
    // init buffer and queue
    DCBuffer buffer {file_name, buffer_size};
    DCQueue queue {buffer, queue_size, matrix_size};
    
    // files outputs
    std::ofstream filters_out(ids_file_out, std::ios::binary);
    std::ofstream filtred_out(filtred_file_out, std::ios::binary);

    if (not filters_out.is_open())
        throw std::runtime_error("Error, cannot create filters output file at location : " + ids_file_out);

    if (not filtred_out.is_open())
        throw std::runtime_error("Error, cannot create unfiltred output file at location : " + filtred_file_out);
    
    while(true)
    {
        auto built_nb = queue.build(buf_delay, queue_delay);
        
        auto filters(queue.filter(_2D, _3D));
        for (const  auto &f : filters)
            filtred_out << std::noskipws << f;

        auto all_datas(queue.get_linear(false, false));
        for (const  auto &v : all_datas)
            filtred_out << std::noskipws << v;

        if (built_nb < queue_size)
            break;
    }
}


/**
 * @brief Method for unfilter and save an input file.
 * 
 * @param file input filtred file name, **only name**, should not contain extension
 * @param ids_file_in filters file name, **only name**, should not contain extension
 * @param unfiltred_file_out file name of output
 * @param buf_delay timer for getting data from buffer
 * @param queue_delay timer for building Queue
 * 
 * @exception std::runtime_error if error when creating file or reading filters ids file
 */
void DCQueue::unfilter_file(const std::string &file_name, const std::string &ids_file_in, const std::string &unfiltred_file_out, double buf_delay, double queue_delay)
{
    std::ifstream filters_in(ids_file_in + ".flt", std::ios::binary);
    std::ofstream unfiltred_out(unfiltred_file_out, std::ios::binary);

    if (not filters_in.is_open())
        throw std::runtime_error("Error, cannot open filters file at location : " + ids_file_in + ".flt");

    if (not unfiltred_out.is_open())
        throw std::runtime_error("Error, cannot create unfiltred output file at location : " + unfiltred_file_out);

    /* reading build info from filters file and reconvert it to base 10 integer */
    uint8_t tmp;
    int i = 0, matrix_size = 0, buffer_size = 0, queue_size = 0;
    for(i = 0; i < sizeof(int); ++i)
    {
        filters_in >> std::noskipws >> tmp;
        matrix_size += tmp * std::pow(256, 3 - i);
    }

    for(i = 0; i < sizeof(int); ++i) 
    {
        filters_in >> std::noskipws >> tmp;
        buffer_size += tmp * std::pow(256, 3 - i);
    }

    for(i = 0; i < sizeof(int); ++i)
    {
        filters_in >> std::noskipws >> tmp;
        queue_size += tmp * std::pow(256, 3 - i);
    }

    /* for decompression, buffer size must be a sub-multiple of total datas = (queue_size * matrix_size * matrix_size)
    then we'll compute a buffer size which is a sub-multiple of total datas, and the closest one from buffer size used for filter. */
    ++queue_size; // +1 for end_matrix from filtred data
    const uint64_t total_datas = queue_size * matrix_size * matrix_size;
    uint64_t up = buffer_size, dw = buffer_size; // we search from buffer_size then increase and decrease, and stop when finding the multiple .
    while(true)
    {
        if (total_datas % up == 0) 
            break;
        ++up;
    }
    while(true)
    {
        if (total_datas % dw == 0)
            break;
        --dw;
    }
    buffer_size = (std::abs(up - buffer_size) < std::abs(dw - buffer_size)) ? up : dw; // the closest sub-multiple to buffer_size
    
    // buffer and queue init
    DCBuffer buffer {file_name + ".flo", buffer_size};
    DCQueue queue {buffer, queue_size, matrix_size};

    int j = 0;
    while(true)
    {
        int built_nb = queue.build(buf_delay, queue_delay);
        
        int filters_size = ((built_nb - 1) * matrix_size) + (matrix_size * matrix_size);
        std::ofstream out("filter_used_" + std::to_string(j) + ".txt");
        std::vector<uint8_t> filters(filters_size);
        for (int i = 0; i < filters_size; ++i)
            filters_in >> std::noskipws >> filters[i];
        
        queue.unfilter(filters, true, true);
        const auto &unfiltred_datas = queue.get_linear(false, true);

        for (const auto &v : unfiltred_datas)
        {
            unfiltred_out << std::noskipws << v;
            out << std::to_string((int)v);
        }

        ++j;
        if (built_nb < queue_size)
            break;
    }

    filters_in.close();
    unfiltred_out.close();
}