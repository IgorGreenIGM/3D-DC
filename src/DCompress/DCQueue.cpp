#include <cmath>
#include <chrono>
#include <thread>
#include <future>
#include <cstring>
#include <assert.h>
#include <iterator>
#include <algorithm>
#include <bits/unique_ptr.h>

#include "../../include/Dcompress/DCQueue.hpp"
#include "../../include/Dcompress/DCUtils.hpp"

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
                tmp_buf[i] = to_base256[sizeof(int) - 1 - i];
        
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
std::vector<uint8_t> DCQueue::z_get(int line, int col) const noexcept
{
    std::vector<uint8_t> out;
    for(auto it = this->begin(); it != this->end() - 1; ++it) // excluding end_matrix
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
    double entropy = .0;
    double length = static_cast<double>((this->counter - 1) * this->matrix_size * this->matrix_size + last_count); // excluding zero-completion values
    std::cout << "computed : " << (int)length << "\n";
    for(const auto &freq : frequency)
    {
        double p_x = static_cast<double>(freq.second) / length;
        entropy -= p_x * std::log2f(p_x);  
    }

    return entropy;
}

/**
 * @brief get entropy of the whole Queue, excluding end_matrix and zero-completion values
 * 
 * @return std::size_t 
 */
double DCQueue::sd() const noexcept
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

    double sum = 0.0;
    for (const auto& val : frequency) {
        sum += val.second;
    }
    double mean = sum / frequency.size();

    // Calculate the sum of squares of differences from the mean
    double sq_diff_sum = 0.0;
    for (const auto& val : frequency) {
        sq_diff_sum += (val.second - mean) * (val.second - mean);
    }

    // Calculate the variance
    double variance = sq_diff_sum / frequency.size();

    // Calculate the standard deviation
    double std_dev = std::sqrt(variance);

    return std_dev;
}


/**
 * @brief method to set DCQueue filters dictionnary
 * 
 * @param dict_path input dictionnary path
 * @exception std::runtime_error if cannot open open dictionnary file at specified path
 */
void DCQueue::set_filters_dict(const std::string &dict_path)
{
    std::ifstream dict_in(dict_path, std::ios::binary);
    if(not dict_in.is_open())
        throw std::runtime_error("Error, cannot open dictionnary file at location : " + dict_path);

    dict_in.unsetf(std::ios::skipws); // desactivate '\n' skip
    std::copy(std::istream_iterator<uint32_t>(dict_in), std::istream_iterator<uint32_t>(), std::back_inserter(this->filters_dict));
}

/**
 * @brief method to set DCQueue filters dictionnary
 * 
 * @param dict_in input dictionnary 
 * @exception std::runtime_error if cannot open open dictionnary file at specified path
 */
void DCQueue::set_filters_dict(const std::vector<uint32_t> &dict_in)
{
    std::copy(dict_in.begin(), dict_in.end(), std::back_inserter(this->filters_dict));
}

/**
 * @brief get filters dicitonnary
 * 
 * @return const std::vector<uint32_t> 
 */
const std::vector<uint32_t> DCQueue::get_filters_dict()
{
    return this->filters_dict;
}

/**
 * @brief clear filters dictionnary
 * 
 */
void DCQueue::clear_filters_dict()
{
    this->filters_dict.clear();
}


/**
 * @brief get all the elements inside the queue, excluding end_matrix.
 * @warning This method return result according to end_matrix.
 * @warning So that the size of the result is exactly the size of data written in DCQueue when building.
 * @return std::vector<uint8_t> 
 */
std::vector<uint8_t> DCQueue::linear_parse() const noexcept
{
    std::vector<uint8_t> out;

    // getting data, excluding end_matrix and last matrix
    for(auto it = this->begin(); it != this->end() - 2; ++it)
        for(int i = 0; i < matrix_size; ++i)
            for(int j = 0; j < matrix_size; ++j)
                out.push_back(it->second[i][j]);
    
    // from end_matrix, computing the size of data written in the last matrix
    int last_count = this->last_data_size();
    if (last_count == 0)
        for(int i = 0; i < matrix_size; ++i)
            for(int j = 0; j < matrix_size; ++j)
                out.push_back((this->end() - 2) ->second[i][j]);
    else
    {
        // getting last datas on each line of last matrix
        for(int i = 0; i < std::floor(last_count / this->matrix_size); ++i)
            for(int j = 0; j < this->matrix_size; ++j)
                out.push_back((this->end() - 2)->second[i][j]);

        // getting last datas
        for(int i = 0; i < last_count - (std::floor(last_count / this->matrix_size) * this->matrix_size); ++i)
            out.push_back((this->end() - 2)->second[std::floor(last_count / this->matrix_size)][i]);
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
 * @brief method to parse the DCQueue, excluding end_matrix
 * 
 * @param parser type of parser 
 * 
 * @return std::vector<uint8_t> containing the parsed result  
 * @exception std::runtime_error if bad parser type specified
 */
std::vector<uint8_t> DCQueue::parse(PARSER parser) const
{
    std::vector<uint8_t> output;

    switch (parser)
    {
        case PARSER::LINEAR_X : 
            for(auto it = this->begin(); it != this->end() - 1; ++it)
                for(int line = 0; line < this->matrix_size; ++line)
                    for(int col = 0; col < this->matrix_size; ++col)
                        output.push_back(it->second[line][col]);
        break;

        case PARSER::LINEAR_Y : 
            for(auto it = this->begin(); it != this->end() - 1; ++it)
                for (int col = 0; col < this->matrix_size; ++col)
                    for (int line = 0; line < this->matrix_size; ++line)
                        output.push_back(it->second[line][col]);
        break;  

        case PARSER::LINEAR_Z :
            for (int line  = 0; line < this->matrix_size; ++line)
                for(int col = 0; col < this->matrix_size; ++col)
                    for (auto it = this->begin(); it != this->end() - 1; ++it)
                        output.push_back(it->second[line][col]);
        break;

        default:
            std::runtime_error("Error, cannot parser with specified parser : " + std::to_string(static_cast<int>(parser)));
        break;
    }
    
    return output;
}


/**
 * @brief from end_matrix, return the size of data written in the last matrix ignoring zero compression values
 * 
 * @return std::size_t 
 */
std::size_t DCQueue::last_data_size() const noexcept
{
    std::size_t last_count(0);

    const auto &end_content ((this->end() - 1)->second.get_linear()); // last matrix
    for (int i = 0; i < end_content.size() and i < sizeof(int); ++i)
        last_count += end_content[i] * std::pow(256, sizeof(int) - 1 - i); // no need to check endianess here, refers to DCQueue::build()

    return last_count;
}


/**
 * @brief method to perform 2D filtering in the DCQueue.
 * @details it consist to apply several 2Dfilters on each matrix in the queue, excluding end_matrix
 * @details 2D filters are applied on each single matrix in the Queue. avaible 2D filters are : NO, SUB, UP, AVERAGE and PAETH
 * @details the method virtually apply each filter, and store to DCQueue filters dictionnary those that maximise datas redundancy(minimize entropy).
 * @details method now supports multithreading ! 
 * @see enum FILTER_MODE
 * 
 * @exception std::runtime_error if the filter dictionnary is full.
 */
void DCQueue::_2D_filter()
{
    if (this->filters_dict.size() + this->matrix_size * this->counter >= this->filters_dict.max_size())
        throw std::runtime_error("error, cannot more filter the Queue, this dictionnary is too much full");

    // 2d filter lambda function, return filtered line 
    auto filter_line2D = [this](uint8_t filter_mode, DCQueue::iterator queue_it, const std::vector<uint8_t> &no_fltr_prev_line, int line) -> std::vector<uint8_t>
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
        std::copy(result.second.begin(), result.second.end(), std::back_inserter(this->filters_dict)); 
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

        // copying threads-local dictionnaries into Queue dictionnary
        for (th_inc = 0; th_inc < nb_threads; ++th_inc)
            std::move(results[th_inc].second.begin(), results[th_inc].second.end(), std::back_inserter(filters_dict));
    }

    // writing 2D filters group id
    this->filters_dict.push_back(FILTER_GROUP_ID::_2D_FILTER);
}


/**
 * @brief method to perform 3D filtering on the Queue.
 * @details it consist to apply several 3D filters on each matrix in the queue, excluding end_matrix
 * @details 3D filters are applied on each specific cell in each matrix of the Queue. avaible 3D filters are : DC_NO, DC_SUB, DC_UP, DC_AVERAGE and DC_PAETH
 * @details the method apply each filter, and stores in the DCqueue filters dictionnary those that maximise datas redundancy(minimize entropy).
 * @warning if there is less than 03 matrix inside the DCQueue, this method does nothing
 * @see enum FILTER_MODE
 * 
 * @exception std::runtime_error if the filter dictionnary is full.
 */
void DCQueue::_3D_filter()
{
    if (this->filters_dict.size() + this->matrix_size * this->matrix_size >= this->filters_dict.max_size())
        throw std::runtime_error("error, cannot more filter the Queue, this dictionnary is too much full");

    if (this->counter < 3)
        return;

    // 3d filter lambda function, return filtred line 
    auto filter_line3D = [this](FILTER_MODE filter_mode, const std::vector<uint8_t> &no_fltr_prev_line, int line, int col) -> std::vector<uint8_t>
    {
        std::vector<uint8_t> filtred_line(this->counter);
        auto z_line(this->z_get(line, col)); 
        
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

    /* we'll now perfrom 3D filtering on each "z-line" and store the filter mode of the one with minimal entropy */
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

            this->filters_dict.push_back(FILTER_MODE::NO_3D + std::distance(filtred_lines.begin(), min));

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

    // writing 3D filter group id in the filter dictionnary
    this->filters_dict.push_back(FILTER_GROUP_ID::_3D_FILTER);
}


/**
 * @brief method for unfiltering the DCQueue 
 * @warning method use internal DCQueue filters dictionnary, then be sure to set the dictionnary 
 * @warning this method will progressively empty the filter dictionnary, then for post-use it's programmer responsability to copy it. 
 * @exception std::runtime_error if invalid Filter id specified
 * @exception std::runtime_error if filters dictionnary is empty
 */
void DCQueue::unfilter()
{
    if (this->filters_dict.empty())
        throw std::runtime_error("cannot perform unfiltering if the filter dictionary is empty");

    // perform 2D unfilter lambda 
    auto unfilter_line2D = [this](uint8_t filter_mode, std::vector<std::pair<int, DCMatrix>>::iterator queue_it, int line, const std::vector<uint8_t> &un_fltrd_prev_line) -> std::vector<uint8_t>
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
    auto unfilter_line3D = [this](uint8_t filter_mode, int line, int col, const std::vector<uint8_t> &un_fltrd_prev_line) -> std::vector<uint8_t>
    {
        const auto z_line(std::move(this->z_get(line, col))); // all deep cells
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

    /**mathematicaly, performing unfilter must be made in the inverted order than the filtering,
     * read the filters docs to understand how it's performed.
    */
    uint32_t filter_inc = 0;
    while (true)
    {
        // retrieve last fitered method id
        uint32_t group_id = this->filters_dict[this->filters_dict.size() - 1];
        this->filters_dict.pop_back();

        if (group_id == FILTER_GROUP_ID::_3D_FILTER) // perform 3D
        {
            filter_inc = this->filters_dict.size() - this->matrix_size * this->matrix_size;

            std::vector<uint8_t> un_fltrd_prev_line(this->counter);
            for (int line = 0; line < this->matrix_size; ++line)
            {
                for (int col = 0; col < this->matrix_size; ++col)
                {
                    // unfilter...
                    std::vector<uint8_t> unfiltred_line(unfilter_line3D(this->filters_dict[filter_inc], line, col, un_fltrd_prev_line));

                    // writing unfiltered cells in queue, excluding the end_matrix
                    int i = 0;
                    for(auto it = this->begin(); it != this->end() - 1; ++it, ++i)
                        it->second[line][col] = unfiltred_line[i];
                    // copying actual line(already unfiltered) of actual matrix as the next "unfiltred previous"
                    std::swap(unfiltred_line, un_fltrd_prev_line);
                    ++filter_inc;
                }
            }
            this->filters_dict.erase(this->filters_dict.end() - this->matrix_size * this->matrix_size, this->filters_dict.end());
        }

        else if (group_id == FILTER_GROUP_ID::_2D_FILTER)
        {
            filter_inc = this->filters_dict.size() - this->matrix_size * this->counter; 

            std::vector<uint8_t> un_fltrd_prev_line(this->matrix_size);
            for(auto queue_it = this->begin(); queue_it != this->end() - 1; ++queue_it) // excluding end_matrix 
            {
                for(int line = 0; line < this->matrix_size; ++line)
                {
                    // unfilter...
                    std::vector<uint8_t> unfiltred_line(unfilter_line2D(filters_dict[filter_inc], queue_it, line, un_fltrd_prev_line)); 
                    
                    // copying actual line(already filtered) of actual matrix as the next "un_filtred previous" 
                    std::copy(unfiltred_line.begin(), unfiltred_line.end(), un_fltrd_prev_line.begin());
                    // writing filtered line into matrix
                    std::copy(unfiltred_line.begin(), unfiltred_line.end(), queue_it->second[line].begin());
                    ++filter_inc;
                }
            }
            this->filters_dict.erase(this->filters_dict.end() - this->matrix_size * this->counter, this->filters_dict.end());
        }
        else
            break;
    }    
    // make sure to erase all
    this->filters_dict.clear(); 
}


/**
 * @brief method to filter and save file
 * 
 * @param src_path source file path 
 * @param dest_path destination file path
 * @param parser type of parser to use
 * @param queue_size size of the DCQueue(byte)
 * @param matrix_size size of the matrix inside DCQueue(byte)
 * @param buffer_size size of the buffer(byte)
 * 
 * @see DCQueue::parse()
 */
void DCQueue::filter_file(const std::string &src_path, const std::string &dest_path, PARSER parser, int queue_size, int matrix_size, int buffer_size)
{
    std::ifstream src_in(src_path, std::ios::binary);
    std::ofstream dest_out(dest_path, std::ios::binary);

    if (not src_in.is_open())
        throw std::runtime_error("Error, cannot open file at location : " + src_path);

    if (not dest_out.is_open())
        throw std::runtime_error("Error, cannot create output file at location : " + dest_path);

    dest_out.unsetf(std::ios::skipws); // unset skipping

    DCBuffer buf(src_path, buffer_size);
    DCQueue queue(buf, queue_size, matrix_size);

    while(true)
    {
        auto built_nb = queue.build(2, 2);

        queue._3D_filter();        

        const auto &all_datas = queue.parse(parser);
        std::copy(all_datas.begin(), all_datas.end(), std::ostream_iterator<uint8_t>(dest_out));

        queue.clear_filters_dict(); // no need the filter dictionnary
        
        if (built_nb < queue_size)
            break;
    }
}

void DCQueue::sort2() noexcept
{
    
}

void DCQueue::sort() noexcept 
{
    std::cout << "inside sorting algorithm . . . " << std::endl;

    const auto &index = [this](int i, int j)
    { return this->size() * i  -((i * (i + 1)) / 2) + j - (i + 1); };

    std::cout << "starting computing probab ility vectors distances \n";
    std::vector<std::unordered_map<uint8_t, double>> pdists;
    for (int i = 0; i < this->size(); ++i)
        pdists.push_back(std::move((*this)[i].second.prob_distribution()));

    std::cout << "starting computing similarity matrixes \n"; 
    // instead of using a 2D symetric matrix, we'll use a linear array
    std::vector<double> sim_array((this->size() * (this->size() - 1)) / 2, .0);
    for (int i = 0; i < this->size(); ++i)
        for (int j = i + 1; j < this->size(); ++j)
            sim_array[index(i, j)] = DCUtils::bhattacharyyan_distance(pdists[i], pdists[j]);

    std::ofstream out("tmp2.txt");

    for (int i = 0; i < this->size(); ++i)
        for (int j = i + 1; j < this->size(); ++j)
            out << "Queue["<< i <<"," << j << "] = " << sim_array[index(i, j)] << "\n";

    out.close();
    // std::cout << "Computing max ...\n";
    // std::vector<double> dists;
    // for(int i = 0; i < this->size(); ++i)
    // {
    //     const auto& matrix = (*this)[i].second;
    //     const auto& zero = DCMatrix(matrix.max(), this->size());
    //     const auto z = DCUtils::bhattacharyyan_distance(zero.prob_distribution(), pdists[i]);

    //     double s(.0);
    //     for (int j = 0; j < this->size(); ++j) 
    //         if (i != j)
    //             s += std::pow(sim_array[index(std::min(i, j), std::max(i, j))], 2.);

    //     auto res = std::sqrt(s + std::pow(z, 2.));
    //     std::cout << "result is : " << res << std::endl;
    //     dists.push_back(res);
    // }

    /* std::vector<DCMatrix> zeros;
    zeros.reserve(this->size());
    for(const auto& matrix : *this)
        zeros.emplace_back(DCMatrix(matrix.second.max(), this->matrix_size));

    std::vector<double> dists(this->size());
    for (const auto& matrix : *this)
    for (int i = 0; i < this->size(); ++i)
        dists[i] = DCUtils::bhattacharyyan_distance(zeros[i].prob_distribution(), (*this)[i].second.prob_distribution());

    */// */std::sort(this->begin(), this->end(), [&dists](const std::pair<int, DCMatrix>& p1, const std::pair<int, DCMatrix>& p2)
    //                                       {
    //                                             return dists[p1.first] < dists[p2.first];
    //                                       });

    /*DCMatrix zero_matrix(max, 0);

    const auto &index = [this](int i, int j)
    { return this->size() * i  -((i * (i + 1)) / 2) + j - (i + 1); };

    std::cout << "starting computing probability vectors distances \n";
    std::vector<std::map<uint8_t, double>> pdists;
    for (int i = 0; i < this->size(); ++i)
        pdists.push_back(std::move((*this)[i].second.prob_distribution()));

    std::cout << "starting computing similarity matrixes \n"; 
    // instead of using a 2D symetric matrix, we'll use a linear array
    std::vector<double> sim_array((this->size() * (this->size() - 1)) / 2, .0);
    for (int i = 0; i < this->size(); ++i)
        for (int j = i + 1; j < this->size(); ++j)
            sim_array[index(i, j)] = DCUtils::bhattacharyyan_distance(pdists[i], pdists[j]);
    
    std::cout << "min is : " << *std::min(sim_array.begin(), sim_array.end()) << std::endl;
    std::cout << "max is : " << *std::max(sim_array.begin(), sim_array.end()) << std::endl;
    std::cout << "starting sorting ...." << std::endl;

    

    std::sort(this->begin(), this->end(), [&sim_array, &index](const std::pair<int, DCMatrix>& p1, const std::pair<int, DCMatrix>& p2) 
                                          {
                                                                                    
                                          });
    */
    std::cout << "finished sorting.... end of sort method \n";
}