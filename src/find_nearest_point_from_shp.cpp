#include "vector_include.h"

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

int test(int a, int b)
{
    typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;

    // 创建 R - Tree
    bgi::rtree<point_t, bgi::quadratic<16>> rtree;

    // 插入点数据
    std::vector<point_t> points = {
        {0.0, 0.0}, {1.0, 1.0}, {2.0, 2.0}, {3.0, 3.0}, {4.0, 4.0},
        {5.0, 5.0}, {6.0, 6.0}, {7.0, 7.0}, {8.0, 8.0}, {9.0, 9.0}
    };
    for (const auto& p : points) {
        rtree.insert(p);
    }

    // 查询最近邻点
    point_t query_point(3.5, 3.5);
    std::vector<point_t> nearest_points;
    rtree.query(bgi::nearest(query_point, 1), std::back_inserter(nearest_points));

    // 输出结果
    if (!nearest_points.empty()) {
        std::cout << "nearest point: (" << bg::get<0>(nearest_points[0]) << ", "
                  << bg::get<1>(nearest_points[0]) << ")" << std::endl;
    } else {
        std::cout << "cann't found nearest point" << std::endl;
    }

    return 0;
}