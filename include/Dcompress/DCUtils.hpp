#ifndef __CLUSTERING_H_INCLUDED__
#define __CLUSTERING_H_INCLUDED__

#include <unordered_map>

class DCUtils
{
    public:
        DCUtils() = delete;
        ~DCUtils() = delete;
        static double bhattacharyyan_distance(const std::unordered_map<uint8_t, double>& p, const std::unordered_map<uint8_t, double>& q) noexcept;
};


#endif //__CLUSTERING_H_INCLUDED__