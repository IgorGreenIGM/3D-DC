# ifndef _DC_BUFFER_H_INCLUDED_
#define _DC_BUFFER_H_INCLUDED_

#include <string>
#include <fstream>
#include <iostream>
#include <bits/unique_ptr.h>


class DCBuffer
{
    public:
        ~DCBuffer();
        DCBuffer(const std::string &file_path, int buffer_size);
        DCBuffer(uint8_t *mem_ptr, int mem_size, int buffer_size);

        bool ready() const noexcept;
        int get_size() const noexcept;
        
        void update_mstream(int new_mem_size, uint8_t *new_mem_ptr) noexcept;

        std::unique_ptr<uint8_t[]> next_chunk_force(int &out_chunk_size) noexcept;
        std::unique_ptr<uint8_t[]> next_chunk(int &out_chunk_size, double delay) noexcept;
        
        // working mode  : f_mode = filestream, m_mode = memory stream mode
        enum class WORK_MODE {f_mode = 0XA, m_mode = 0XF};

    private:
        std::unique_ptr<uint8_t[]> mem_buf; // internal data handler, fixed size
        int buffer_size; // size of the data handler

        uint8_t *in_mstream; // input memory stream
        int next_mrd_pos; // next read memory stream position
        int m_stream_size; // input memory stream size

        std::ifstream in_fstream; // input file stream

        WORK_MODE work_mode; // working mode of the buffer, filestream mode or memstream mode

        int countdown; // avaible places in the buffer, -1 if there is no more datas to read
        bool locked; // the buffer state, locked=true if not full, and false else.
};

#endif // _DC_BUFFER_H_INCLUDED_