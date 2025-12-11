#ifndef _DC_MAP_INCLUDED_
#define _DC_MAP_INCLUDED_

#include <ostream>
#include <queue>
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

        //setters

        inline auto get_x() const noexcept {return this->x;};
        inline auto get_y() const noexcept {return this->y;};
        inline auto get_z() const noexcept {return this->z;};

        // operators overloading

        inline friend bool operator!=(const DCPoint &A, const DCPoint &B) { return A==B ? false : true;};
        inline friend bool operator==(const DCPoint &A, const DCPoint &B) { return A.x == B.x and A.y == B.y and A.z == B.z;};

        inline friend DCPoint operator+(const DCPoint &A, const DCPoint &B) { return DCPoint{A.x + B.x, A.y + B.y, A.z + B.z};}
        inline friend DCPoint operator*(const DCPoint &A, const DCPoint &B) { return DCPoint{A.x * B.x, A.y * B.y, A.z * B.z};}
        inline friend DCPoint operator*(const DCPoint &A, int multiplier) { return DCPoint{multiplier * A.x, multiplier * A.y, multiplier * A.z}; }

        inline friend std::ostream& operator<<(std::ostream &stream, const DCPoint &A) 
        { 
            stream << "(" << A.x << "," << A.y << "," << A.z << ")";
            return stream;
        }
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

        DCPoint to_user(const DCPoint &point) const noexcept;
        DCPoint to_local(const DCPoint &point) const noexcept;

        std::vector<uint8_t> range_parse(const DCPoint &_A, const DCPoint &_B) const noexcept;
        std::vector<DCPoint> eq_neighbours(const DCPoint &point, int distance) const noexcept;
        std::vector<DCPoint> eq_points(uint8_t value) const noexcept;
        std::vector<DCPoint> eq_pointss(uint8_t value) const noexcept;

    private : 
        const DCQueue &queue;
        DCPoint origin;
        DCPoint I, J, K;
};

enum DIRECTION { 
                F, B, U, D, J,     // front, back, up, down, jump
                FU, FD, BU, BD, // front up, front down, back up, back down
                NM, S              // next matrix, static
                };

#endif // _DC_MAP_INCLUDED_