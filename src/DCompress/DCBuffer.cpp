#include <thread>
#include <chrono>
#include <cstring>

#include "../../include/Dcompress/DCBuffer.h"


/**
 * @brief Construct a new DCBuffer::DCBuffer object from an input specified file path
 * 
 * @param file_path input file path
 * @param buffer_size wanted size of the buffer
 * @exception std::runtime_error if can't open file specified at path
 */
DCBuffer::DCBuffer(const std::string &file_path, int buffer_size)
{
    this->buffer_size = buffer_size; // buffer size
    this->in_fstream = std::ifstream(file_path.c_str(), std::ios::binary | std::ios::in);  // opening file stream

    if(not in_fstream.is_open())
        throw std::runtime_error("Error, impossible to open file specified at location : " + file_path);
    
    this->locked = true;
    this->next_mrd_pos = 0;
    this->m_stream_size = 0;
    this->work_mode = f_mode;
    this->in_mstream = nullptr;
    this->countdown = this->buffer_size;
}


/**
 * @brief Construct a new DCBuffer::DCBuffer object from an input memory buffer(location)
 * 
 * @param mem_ptr input memory buffer
 * @param mem_size input memory buffer size
 * @param buffer_size wanted size of the buffer
 */
DCBuffer::DCBuffer(uint8_t *mem_ptr, int mem_size, int buffer_size)
{
    this->buffer_size = buffer_size; // buffer size
    this->in_mstream = mem_ptr; // storing input memory stream 

    this->locked = true;
    this->next_mrd_pos = 0;
    this->work_mode = m_mode;
    this->m_stream_size = mem_size;
    this->countdown = this->buffer_size;
}


/**
 * @brief Destroy the DCBuffer::DCBuffer object
 * 
 */
DCBuffer::~DCBuffer()
{
    if(this->in_fstream.is_open())
        this->in_fstream.close();
}


/**
 * @brief method to get the chunk of data from streams(file or memory), force mode.
 * @details if the datas are not avaible at the moment of call, this method will force the buffer unlocking, then the output chunk length will be 
 * stored in the out_chunk_size var.
 * @note it is the programmer responsability to manage the output, cause it is moved from internal datas handler at return
 * 
 * @param out_chunk_size output value of the chunk size.
 * @return std::unique_ptr<uint8_t[]> 
 */
std::unique_ptr<uint8_t[]> DCBuffer::next_chunk_force(int &out_chunk_size) noexcept
{
    // cause this method move datas from internal handler, it's necessarry to each call from necessary memory
    this->mem_buf.reset(new uint8_t[this->buffer_size]); // buffer memory allocation

    if (this->work_mode == f_mode) // file stream working mode
    {   
        // copying stream data in internal data handler
        int i = 0;
        while (this->in_fstream >> std::noskipws >> this->mem_buf[i] and this->countdown != 0) 
        {
            ++i;
            --this->countdown;
        }

        if(this->countdown != 0) // meaning that there is no more data to read in file stream but the buffer is not full
        {
            this->locked = false; // forcing unlocking the buffer
            this->countdown = -1; // no  more datas...
            out_chunk_size = i;
        }
        else
        {
            this->locked = false; // unlocking the buffer...
            this->countdown = this->buffer_size; // re-setting countdown
            out_chunk_size = this->buffer_size;
        }
    }
    else // memory stream working mode
    {
        // if the rest of datas to read in fewer than the buffer size
        if((this->m_stream_size - this->next_mrd_pos) < this->buffer_size)
        {

            std::memcpy(this->mem_buf.get() + this->next_mrd_pos, this->in_mstream + this->next_mrd_pos, this->m_stream_size);
            this->locked = false; // force buffer unlocking
            this->countdown = -1;
            this->next_mrd_pos += this->m_stream_size;
            out_chunk_size = this->m_stream_size;
        }
        else
        {
            std::memcpy(this->mem_buf.get() + this->next_mrd_pos, this->in_mstream + this->next_mrd_pos, this->buffer_size);
            this->locked = false;
            this->countdown = this->buffer_size;
            this->next_mrd_pos += this->buffer_size;
            out_chunk_size = this->buffer_size;
        }
    }

    return std::move(this->mem_buf);
}


/**
 * @brief method to get the chunk of data from streams(file or memory).
 * @note it is the programmer responsability to manage the output, cause it is move from internal datas handler before returned
 * @note you can get the state of the buffer with the ready() method.
 * @see DCBuffer::ready()
 * 
 * @param out_chunk_size output value of the chunk size.
 * @param delay the delay time until force memory buffer unlock(unit = sec) and release datas
 * @return std::unique_ptr<uint8_t[]>
 */
std::unique_ptr<uint8_t[]> DCBuffer::next_chunk(int &out_chunk_size, double delay) noexcept
{   
    // cause this method move datas from internal handler, it's necessarry to each call from necessary memory
    this->mem_buf.reset(new uint8_t[this->buffer_size]); // buffer memory allocation

    if (this->work_mode == f_mode) // file stream working mode
    {   
        // copying avaible stream data in internal data handler
        int i = 0;
        while (this->in_fstream >> std::noskipws >> this->mem_buf[i] and this->countdown != 0) // reading avaible datas
        {
            ++i;
            --this->countdown;
        }
        // cause the 02 conditions in while loops, there is i+1 reads in the file stream
        this->in_fstream.seekg(-1, std::ios::cur); 

        auto sc = std::chrono::steady_clock();
        auto start = sc.now();
        while(this->countdown != 0) // while the buffer is not full
        {
            this->in_fstream.clear(); // clearing eof error, so be able to read next added datas
            if(this->in_fstream >> std::noskipws >> mem_buf[i])
            {
                ++i;
                --this->countdown;
            }

            if(static_cast<std::chrono::duration<double>>(sc.now() - start).count() > delay) // if delay expired
            {                
                this->locked = false; // force buffer unlocking
                out_chunk_size = i;

                return std::move(this->mem_buf);
            }
        }

        // if the buffer is full
        this->locked = false; // unlocking the buffer...
        this->countdown = this->buffer_size; // re-setting countdown
        out_chunk_size = this->buffer_size;
    }
    else // memory stream working mode
    {
        int i = 0;
        while(i + this->next_mrd_pos < this->m_stream_size and this->countdown != 0)
        {   
            this->mem_buf[i] = this->in_mstream[i];
            ++i;
            --this->countdown;
        }

        auto sc = std::chrono::steady_clock();
        auto start = sc.now();
        while(this->countdown != 0) // while the buffer is not full
        {
            if(i + this->next_mrd_pos < this->m_stream_size)
            {
                ++i;
                --this->countdown;
            }

            if(static_cast<std::chrono::duration<double>>(sc.now() - start).count() > delay) // if delay expired
            {
                this->locked = false; // force buffer unlocking
                out_chunk_size = i;
                this->next_mrd_pos += i;
                return std::move(this->mem_buf);
            }            
        }
        // if the buffer is full
        this->locked = false; // unlocking the buffer...
        this->countdown = this->buffer_size; // re-setting countdown
        out_chunk_size = this->buffer_size;
        this->next_mrd_pos += this->buffer_size;
    }
    return std::move(this->mem_buf);
}


/**
 * @brief method to get the size of the buffer
 * 
 * @return size of the buffer
 */
int DCBuffer::get_size() const noexcept
{
    return this->buffer_size;
}

/**
 * @brief get the state of the buffer
 * 
 * @return true if the buffer is ready, ie the buffer is full
 * @return false if the buffer is not ready, ie the buffer is not yet full
 */
bool DCBuffer::ready() const noexcept
{
    return (!this->locked);
}

/**
 * @brief update memory stream
 * @param new_mem_size new input memory buffer size 
 * @param new_mem_ptr new input memory buffer. nullptr by default if the stream doesn't changed
 */
void DCBuffer::update_mstream(int new_mem_size, uint8_t *new_mem_ptr = nullptr) noexcept
{
    if(new_mem_ptr != nullptr)
        this->in_mstream = new_mem_ptr;

    this->m_stream_size = new_mem_size;
}