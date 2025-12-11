
#include <cmath>
#include <numeric>
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <unordered_map>

#include "../../include/Dcompress/DCUtils.hpp"


double DCUtils::bhattacharyyan_distance(const std::unordered_map<uint8_t, double>& pdist1, const std::unordered_map<uint8_t, double>& pdist2) noexcept
{
    double sum(.0);
    for (int i = 0; i < 256; ++i)
    {
        auto it1 = pdist1.find(i);
        auto it2 = pdist2.find(i);

        if (it1 != pdist1.end() && it2 != pdist2.end()) 
            sum += std::sqrt(it1->second * it2->second);
    }

    if (sum == 0.0) 
        return std::numeric_limits<double>::infinity();

    return -std::log(sum);
}