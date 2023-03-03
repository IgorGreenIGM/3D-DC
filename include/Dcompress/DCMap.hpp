#ifndef _DC_MAP_INCLUDED_
#define _DC_MAP_INCLUDED_

#include <ostream>
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

        auto get_x() const noexcept {return this->x;};
        auto get_y() const noexcept {return this->y;};
        auto get_z() const noexcept {return this->z;};

        // operators overloading

        friend bool operator!=(const DCPoint &A, const DCPoint &B) { return A==B ? false : true;};
        friend bool operator==(const DCPoint &A, const DCPoint &B) { return A.x == B.x and A.y == B.y and A.z == B.z;};

        friend DCPoint operator+(const DCPoint &A, const DCPoint &B) { return DCPoint{A.x + B.x, A.y + B.y, A.z + B.z};}
        friend DCPoint operator*(const DCPoint &A, const DCPoint &B) { return DCPoint{A.x * B.x, A.y * B.y, A.z * B.z};}
        friend DCPoint operator*(const DCPoint &A, int multiplier) { return DCPoint{multiplier * A.x, multiplier * A.y, multiplier * A.z}; }
        
        friend std::ostream& operator<<(std::ostream &stream, const DCPoint &A) 
        { 
            stream << "(" << A.get_x() << "," << A.get_y() << "," << A.get_z() << ")";
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

        DCPoint to_local(const DCPoint &point) const noexcept;
        DCPoint to_user(const DCPoint &point) const noexcept;
        std::vector<uint8_t> range_parse(const DCPoint &_A, const DCPoint &_B);
        std::vector<DCPoint> eq_neighbours(const DCPoint &point, int distance);

    private : 
        const DCQueue &queue;
        DCPoint origin;
        DCPoint I, J, K;
};

/**
 * @details DCQueue local coordinate systems is as follw : 
 * origin=(matrix=0,line=0,column=0)
 * I:x_axis direction=(matrix=1,line=0,column=0)
 * J:y_axis direction=(matrix=0,line=1,column=0)
 * Z:z_axis direction=(matrix=0,line=0,column=1)
 */

#endif // _DC_MAP_INCLUDED_