#include "../BuildRectangleGeometry.h"

#include <cassert>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <set>
#include <utility>

using BuildRectangleGeometry::Generate;
using BuildRectangleGeometry::Point;

namespace
{
    void ExpectOrder(const std::vector<Point>& actual, const std::vector<Point>& expected)
    {
        assert(actual.size() == expected.size());
        for (std::size_t i = 0; i < actual.size(); ++i)
        {
            assert(actual[i].x == expected[i].x);
            assert(actual[i].y == expected[i].y);
        }
    }

    void ExpectGrid(const std::vector<Point>& points, const std::vector<int>& xs,
        const std::vector<int>& ys)
    {
        std::set<std::pair<int, int>> expected;
        for (int x : xs)
            for (int y : ys) expected.insert(std::make_pair(x, y));
        std::set<std::pair<int, int>> actual;
        for (const Point& point : points) actual.insert(std::make_pair(point.x, point.y));
        assert(points.size() == actual.size());
        assert(actual == expected);
    }

    void ExpectAdjacent(const std::vector<Point>& points, int stepX, int stepY)
    {
        for (std::size_t i = 1; i < points.size(); ++i)
        {
            const int dx = std::abs(points[i].x - points[i - 1].x);
            const int dy = std::abs(points[i].y - points[i - 1].y);
            assert((dx == stepX && dy == 0) || (dx == 0 && dy == stepY));
        }
    }

    void TestNearestSidesAndOrder()
    {
        std::vector<Point> points;
        // A 3 by 3 rectangle with unequal building dimensions and one grid
        // square of spacing. Each side selects the corresponding sweep axis.
        assert(Generate(160, 320, 256, 448, 2, 3, 1, 140, 384, 999, points));
        ExpectOrder(points, {{256,448}, {256,384}, {256,320},
                            {208,320}, {208,384}, {208,448},
                            {160,448}, {160,384}, {160,320}});
        assert(Generate(160, 320, 256, 448, 2, 3, 1, 276, 384, 999, points));
        ExpectOrder(points, {{160,448}, {160,384}, {160,320},
                            {208,320}, {208,384}, {208,448},
                            {256,448}, {256,384}, {256,320}});
        assert(Generate(160, 320, 256, 448, 2, 3, 1, 208, 300, 999, points));
        ExpectOrder(points, {{256,448}, {208,448}, {160,448},
                            {160,384}, {208,384}, {256,384},
                            {256,320}, {208,320}, {160,320}});
        assert(Generate(160, 320, 256, 448, 2, 3, 1, 208, 468, 999, points));
        ExpectOrder(points, {{256,320}, {208,320}, {160,320},
                            {160,384}, {208,384}, {256,384},
                            {256,448}, {208,448}, {160,448}});
        ExpectAdjacent(points, 48, 64);

        // With two layers, the first direction must reverse to leave the
        // final point at the corner nearest the builder.
        assert(Generate(0, 0, 32, 64, 2, 2, 0, -16, 0, 999, points));
        ExpectOrder(points, {{32,0}, {32,32}, {32,64}, {0,64}, {0,32}, {0,0}});
        assert(Generate(0, 0, 32, 64, 2, 2, 0, -16, 64, 999, points));
        ExpectOrder(points, {{32,64}, {32,32}, {32,0}, {0,0}, {0,32}, {0,64}});
        assert(Generate(0, 0, 64, 32, 2, 2, 0, 54, -16, 999, points));
        ExpectOrder(points, {{64,32}, {32,32}, {0,32}, {0,0}, {32,0}, {64,0}});

        // A builder inside the area still selects the closest side. Equal
        // side distances use the documented left-first tie break.
        assert(Generate(0, 0, 64, 96, 2, 2, 0, 8, 48, 999, points));
        assert(points.front().x == 64 && points.front().y == 96);
        assert(points.back().x == 0 && points.back().y == 0);
        ExpectAdjacent(points, 32, 32);
        assert(Generate(0, 0, 64, 64, 2, 2, 0, 32, 32, 999, points));
        assert(points[0].x == 64 && points[0].y == 64);
        assert(points[1].x == 64 && points[1].y == 32);
        assert(points.back().x == 0 && points.back().y == 0);
    }

    void TestAllDragQuadrants()
    {
        std::vector<Point> points;
        for (int directionX : {-1, 1})
        {
            for (int directionY : {-1, 1})
            {
                const int endX = 160 + directionX * 96;
                const int endY = 320 + directionY * 128;
                const std::vector<int> xs = {160, 160 + directionX * 48, endX};
                const std::vector<int> ys = {320, 320 + directionY * 64, endY};
                const int left = directionX < 0 ? endX : 160;
                const int right = directionX < 0 ? 160 : endX;
                const int top = directionY < 0 ? endY : 320;
                assert(Generate(160, 320, endX, endY, 2, 3, 1,
                    left - 16, top + 64, 999, points));
                ExpectGrid(points, xs, ys);
                ExpectAdjacent(points, 48, 64);
                assert(points.front().x == right);
                assert(points.back().x == left && points.back().y == top);
            }
        }
        // Remainders belong at the moving end. Normalising the rectangle
        // before generating would incorrectly move this original grid origin.
        assert(Generate(101, 203, 0, 40, 2, 3, 1, -20, 139, 999, points));
        ExpectGrid(points, {101,53,5}, {203,139,75});
        ExpectAdjacent(points, 48, 64);
    }

    void TestDegenerateRectangles()
    {
        std::vector<Point> points;
        assert(Generate(0, 0, 96, 0, 2, 3, 0, -16, 0, 999, points));
        ExpectOrder(points, {{96,0}, {64,0}, {32,0}, {0,0}});
        assert(Generate(0, 0, 96, 0, 2, 3, 0, 112, 0, 999, points));
        ExpectOrder(points, {{0,0}, {32,0}, {64,0}, {96,0}});
        assert(Generate(0, 0, 0, 96, 3, 2, 0, 0, -16, 999, points));
        ExpectOrder(points, {{0,96}, {0,64}, {0,32}, {0,0}});
        assert(Generate(0, 0, 0, 96, 3, 2, 0, 0, 112, 999, points));
        ExpectOrder(points, {{0,0}, {0,32}, {0,64}, {0,96}});
        assert(Generate(17, 29, 17, 29, 2, 3, 0, 200, 200, 1, points));
        ExpectOrder(points, {{17,29}});
        assert(Generate(17, 29, 48, 76, 2, 3, 0, 200, 200, 1, points));
        ExpectOrder(points, {{17,29}});
    }

    void TestSpacingAndFootprints()
    {
        std::vector<Point> points;
        assert(Generate(0, 0, 160, 160, 2, 3, 0, -10, 80, 999, points));
        ExpectGrid(points, {0,32,64,96,128,160}, {0,48,96,144});
        ExpectAdjacent(points, 32, 48);
        assert(Generate(0, 0, 160, 160, 2, 3, 1, -10, 80, 999, points));
        ExpectGrid(points, {0,48,96,144}, {0,64,128});
        ExpectAdjacent(points, 48, 64);
        assert(Generate(0, 0, 160, 160, 2, 3, 10, -10, 80, 999, points));
        ExpectOrder(points, {{0,0}});
        assert(Generate(0, 0, 64, 160, 1, 4, 2, -10, 80, 999, points));
        ExpectGrid(points, {0,48}, {0,96});
        ExpectAdjacent(points, 48, 96);
    }

    void TestCapacityAndInvalidInputs()
    {
        std::vector<Point> points;
        assert(Generate(0, 0, 64, 64, 2, 2, 0, -10, 32, 9, points));
        assert(points.size() == 9);
        assert(!Generate(0, 0, 64, 64, 2, 2, 0, -10, 32, 8, points));
        assert(points.empty());
        assert(Generate(0, 0, 998 * 16, 0, 1, 1, 0, -10, 0, 999, points));
        assert(points.size() == 999);
        assert(!Generate(0, 0, 999 * 16, 0, 1, 1, 0, -10, 0, 999, points));
        assert(points.empty());
        assert(!Generate(0, 0, 31 * 16, 31 * 16, 1, 1, 0, -10, 0, 999, points));
        assert(points.empty());
        const int invalid[][4] = {
            {0, 2, 0, 999}, {-1, 2, 0, 999}, {2, 0, 0, 999},
            {2, -1, 0, 999}, {2, 2, -1, 999}, {2, 2, 0, 0}, {2, 2, 0, -1}
        };
        for (const auto& values : invalid)
        {
            points.push_back({1,2});
            assert(!Generate(0, 0, 0, 0, values[0], values[1], values[2],
                0, 0, values[3], points));
            assert(points.empty());
        }
    }

    void TestCoordinateExtremes()
    {
        std::vector<Point> points;
        assert(!Generate(INT_MIN, INT_MIN, INT_MAX, INT_MAX, 1, 1, 0,
            INT_MIN, INT_MAX, 999, points));
        assert(points.empty());
        assert(Generate(INT_MIN, INT_MIN, INT_MAX, INT_MAX, INT_MAX, INT_MAX,
            INT_MAX, INT_MAX, INT_MIN, 1, points));
        ExpectOrder(points, {{INT_MIN,INT_MIN}});
        assert(Generate(INT_MAX - 100, INT_MIN, INT_MAX, INT_MIN + 47,
            2, 3, 0, INT_MIN, INT_MAX, 999, points));
        ExpectGrid(points, {INT_MAX-100, INT_MAX-68, INT_MAX-36, INT_MAX-4}, {INT_MIN});
    }
}

int main()
{
    TestNearestSidesAndOrder();
    TestAllDragQuadrants();
    TestDegenerateRectangles();
    TestSpacingAndFootprints();
    TestCapacityAndInvalidInputs();
    TestCoordinateExtremes();
    std::cout << "Rectangle build geometry tests passed\n";
}
