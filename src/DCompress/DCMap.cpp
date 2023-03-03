#include <tuple>

#include "../../include/Dcompress/DCMap.hpp"

/**
 * @brief Construct a new DCMap::DCMap object.
 * @note this constructor automatically set coordinate system as follow : 
 * @note origin=(matrix:0, line:last, column:0)
 * @note I=(matrix:0, line:last, column:1)
 * @note J=(matrix:1, line:last, column:0)
 * @note K=(matrix:0, line:last-1, column:0).
 * @note Which is a direct orthonormal cartesian system (O, I, J, K).
 * 
 * @param queue reference to DCQueue that DCMap will overlay
 */
DCMap::DCMap(const DCQueue &queue) : queue(queue)
{
    this->origin = DCPoint(0, this->queue.get_matrix_size() - 1, 0);
    this->I = DCPoint(0, this->queue.get_matrix_size() - 1, 1);
    this->J = DCPoint(1, this->queue.get_matrix_size() - 1, 0);
    this->K = DCPoint(0, this->queue.get_matrix_size() - 2, 0);
}

/**
 * @brief Construct a new DCMap::DCMap object
 * @note this constructor automatically set axis as follow : 
 * @note I=(matrix:origin, line:origin, column:origin+1)
 * @note J=(matrix:origin+1, line:origin, column:origin)
 * @note K=(matrix:origin, line:origin-1, column:origin)
 * @note Which define a direct orthonormal cartesian system (O, I, J, K).
 * 
 * @param queue reference to DCQueue that DCMap will overlay
 * @param origin origin of the coordinate system
 */
DCMap::DCMap(const DCQueue &queue, DCPoint origin) : queue(queue), origin(origin)
{
    this->I = DCPoint(this->origin.x, this->origin.y, this->origin.z + 1);
    this->J = DCPoint(this->origin.x + 1, this->origin.y, this->origin.z);
    this->K = DCPoint(this->origin.x, this->origin.y - 1, this->origin.z);
}

/**
 * @brief Construct a new DCMap::DCMap object
 * 
 * @param queue reference to DCQueue that DCMap will overlay
 * @param origin origin of the coordinate system
 * 
 * @param _I I point I the coordinate system 
 * @param _J J point J the coordinate system
 * @param _K K point K the coordinate system
 * @see DCMap::DCMap
 */
DCMap::DCMap(const DCQueue &queue, const DCPoint &origin, const DCPoint &_I, 
                                                          const DCPoint &_J, 
                                                          const DCPoint &_K) : queue(queue), origin(origin), I(_I), J(_J), K(_K)
{
}


/**
 * @brief convert input DCPoint from user system coordinate to DCQueue local coordinate system.
 * @note this solution was obtained mathematically.
 * @param point input point(in user sytem coordinate)
 * 
 * @return DCPoint converted point
 */
DCPoint DCMap::to_local(const DCPoint &point) const noexcept
{
    return 
    { origin.x + point.x * (I.x - origin.x) + point.y * (J.x - origin.x) + point.z * (K.x - origin.x),
      origin.y + point.x * (I.y - origin.y) + point.y * (J.y - origin.y) + point.z * (K.y - origin.y),
      origin.z + point.x * (I.z - origin.z) + point.y * (J.z - origin.z) + point.z * (K.z - origin.z)
    };
}

/**
 * @brief convert input DCPoint DCQueue local coordinate system to user system coordinate.
 * 
 * @note this solution was obtained mathematically.
 * @param point input point(in local sytem coordinate)
 * 
 * @return converted point
 */
DCPoint DCMap::to_user(const DCPoint &point) const noexcept
{
    auto det = (I.x - origin.x)*((K.z - origin.z)*(J.y - origin.y) - (J.z - origin.z)*(K.y - origin.y)) + (J.x - origin.x)*((I.z - origin.z)*(K.y - origin.y) - (K.z - origin.z)*(I.y - origin.y)) + (K.x - origin.x)*((J.z - origin.z)*(I.y - origin.y) - (I.z - origin.z)*(J.y - origin.y));
    return
    { ((J.x - origin.x)*((point.z - origin.z)*(K.y - origin.y) - (point.y - origin.y)*(K.z - origin.z)) + (K.x - origin.x)*((point.y - origin.y)*(J.z - origin.z) - (point.z - origin.z)*(J.y - origin.y)) + (point.x - origin.x)*((K.z - origin.z)*(J.y - origin.y) - (J.z - origin.z)*(K.y - origin.y))) / det,
      ((I.x - origin.x)*(-(point.z - origin.z)*(K.y - origin.y) + (point.y - origin.y)*(K.z - origin.z)) + (K.x - origin.x)*(-(point.y - origin.y)*(I.z - origin.z) + (point.z - origin.z)*(I.y - origin.y)) + (point.x - origin.x)*(-(K.z - origin.z)*(I.y - origin.y) + (I.z - origin.z)*(K.y - origin.y))) / det,
      ((I.x - origin.x)*((point.z - origin.z)*(J.y - origin.y) - (point.y - origin.y)*(J.z - origin.z)) + (J.x - origin.x)*((point.y - origin.y)*(I.z - origin.z) - (point.z - origin.z)*(I.y - origin.y)) + (point.x - origin.x)*((J.z - origin.z)*(I.y - origin.y) - (I.z - origin.z)*(J.y - origin.y))) / det
    };
}


/**
 * @brief Method to get all elements inside the matrix on a specific segment [AB]
 * @note based on Bresenham Algorithm
 * @note A and B are included
 * 
 * @param _A start point
 * @param _B end point
 * @return std::vector<uint8_t> parsed result between A and B
 */
std::vector<uint8_t> DCMap::range_parse(const DCPoint &_A, const DCPoint &_B)
{
    // transform points coordinates to DCQueue coord-sys
    auto A = this->to_local(_A);
    auto B = this->to_local(_B);

    std::vector<uint8_t> out; // output
    out.push_back(queue.at(A.x).second.at(A.y, A.z)); // start by A point included

    int dx = std::abs(B.x - A.x), dy = std::abs(B.y - A.y), dz = std::abs(B.z - A.z);
    int xs = (B.x > A.x) ? 1 : -1;
    int ys = (B.y > A.y) ? 1 : -1;
    int zs = (B.z > A.z) ? 1 : -1;

    // X-axis as driving axis
    if (dx >= dy and dx >= dz)
    {
        int p1 = 2 * dy - dx;
        int p2 = 2 * dz - dx;
        while (A.x != B.x) 
        {
            A.x += xs;
            if (p1 >= 0) 
            {
                A.y += ys;
                p1 -= 2 * dx;
            }
            if (p2 >= 0) 
            {
                A.z += zs;
                p2 -= 2 * dx;
            }
            p1 += 2 * dy;
            p2 += 2 * dz;
            out.push_back(queue.at(A.x).second.at(A.y, A.z));
        }
    }
    // Y-axis as driving axis
    else if (dy >= dx && dy >= dz) 
    {
        int p1 = 2 * dx - dy;
        int p2 = 2 * dz - dy;
        while (A.y != B.y) 
        {
            A.y += ys;
            if (p1 >= 0) 
            {
                A.x += xs;
                p1 -= 2 * dy;
            }
            if (p2 >= 0) 
            {
                A.z += zs;
                p2 -= 2 * dy;
            }
            p1 += 2 * dx;
            p2 += 2 * dz;
            out.push_back(queue.at(A.x).second.at(A.y, A.z));
        }
    }
    // Z-axis as driving axis
    else
    {
        int p1 = 2 * dy - dz;
        int p2 = 2 * dx - dz;
        while (A.z != B.z) 
        {
            A.z += zs;
            if (p1 >= 0) 
            {
                A.y += ys;
                p1 -= 2 * dz;
            }
            if (p2 >= 0) 
            {
                A.x += xs;
                p2 -= 2 * dz;
            }
            p1 += 2 * dy;
            p2 += 2 * dx;            
            out.push_back(queue.at(A.x).second.at(A.y, A.z));
        }
    }

    return out;
}

/**
 * @brief method to get all neighbours points that have the same value as the input 
 * @param point input point 
 * @param neighbours_nb radius distance
 * @note input point is included will be included in the output
 *  * 
 * @return std::vector<DCPoint> neighbours points
 */
std::vector<DCPoint> DCMap::eq_neighbours(const DCPoint &point, int distance)
{
    auto p = this->to_local(point);
    std::vector<DCPoint> out; // output

    for (int mat = -distance; mat <= distance; ++mat) 
    {
        if (p.x + mat >= this->queue.get_matrix_nb() or p.x + mat < 0)
            continue;

        for (int line = -distance; line <= distance; ++line)    
        {
            if (p.y + line >= this->queue.get_matrix_size() or p.y + line < 0)
                continue;

            for (int col = -distance; col <= distance; ++col)
            {
                if (p.z + col >= this->queue.get_matrix_size() or p.z + col < 0)
                    continue;

                else
                    if (this->queue.at(p.x).second.at(p.y, p.z) == this->queue.at(p.x + mat).second.at(p.y + line, p.z + col))
                        out.push_back(this->to_user({p.x + mat, p.y + line, p.z + col}));
            }
        }    
    }

    return out;
}