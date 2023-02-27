#ifndef _DC_MAP_INCLUDED_
#define _DC_MAP_INCLUDED_

#include "./DCQueue.hpp"


class DCPoint
{
    private : 
        int x, y, z;

    public:
        DCPoint() : x(0), y(0), z(0) {}
        DCPoint(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
        DCPoint(std::initializer_list<int> lst) : x(*lst.begin()), y(*(lst.begin() + 1)), z(*(lst.begin() + 2)){}

        friend class DCMap;
        friend DCPoint operator+(const DCPoint &A, const DCPoint &B) { return DCPoint{A.x + B.x, A.y + B.y, A.z + B.z};}
        friend DCPoint operator*(const DCPoint &A, const DCPoint &B) { return DCPoint{A.x * B.x, A.y * B.y, A.z * B.z};}
        friend DCPoint operator*(const DCPoint &B, int multiplier) { return DCPoint{multiplier * B.x, multiplier * B.y, multiplier * B.z}; }
};

/**
 * @brief System Coordinate that overlay the DCQueue to simplify manipulation.
 * @class DCMap
 * @note I, J and K points are used to define axis directions : 
 * @note OI→ vector indicate x axis direction.
 * @note OJ→ vector indicate y axis direction. 
 * @note OK→ vector indicate z axis direction.
 * 
 */
class DCMap
{
    public :
        explicit DCMap(const DCQueue &queue);
        DCMap(const DCQueue &queue, DCPoint origin);
        DCMap(const DCQueue &queue, const DCPoint &origin, const DCPoint &_x, const DCPoint &_y, const DCPoint &_z);

        DCPoint transform(const DCPoint &point) const noexcept;
        std::vector<uint8_t> range_parse(DCPoint A, DCPoint B);
        // std::vector<uint8_t> line_parse();

    private : 
        const DCQueue &queue;
        DCPoint origin;
        DCPoint I, J, K;
};


#endif // _DC_MAP_INCLUDED_