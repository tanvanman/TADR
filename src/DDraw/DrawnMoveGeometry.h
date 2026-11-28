#pragma once
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

// No engine dependencies: the same geometry is exercised by the native tests.
namespace DrawnMoveGeometry
{
    struct Point { double x, y; };
    inline double DistanceSquared(Point a, Point b) {
        const double x = a.x-b.x, y = a.y-b.y;
        return x*x+y*y;
    }
    inline std::vector<Point> Resample(const std::vector<Point>& path, std::size_t count) {
        std::vector<Point> result;
        if (path.empty() || count == 0) return result;
        std::vector<double> lengths(path.size(), 0.0);
        for (std::size_t i=1; i<path.size(); ++i)
            lengths[i] = lengths[i-1] + std::sqrt(DistanceSquared(path[i-1],path[i]));
        const double total = lengths.back();
        std::size_t segment = 1;
        for (std::size_t i=0; i<count; ++i) {
            const double distance = total * (count == 1 ? 0.5 : double(i)/double(count-1));
            while (segment < path.size() && lengths[segment] <= distance) ++segment;
            if (segment == path.size()) { result.push_back(path.back()); continue; }
            const double span = lengths[segment]-lengths[segment-1];
            const double t = span > 0 ? (distance-lengths[segment-1])/span : 0;
            result.push_back({path[segment-1].x + t*(path[segment].x-path[segment-1].x),
                              path[segment-1].y + t*(path[segment].y-path[segment-1].y)});
        }
        return result;
    }
    // Greedy assignment followed by bounded pair improvements. O(n^2), no
    // cubic solver or work per mouse event; stable ties make results repeatable.
    inline std::vector<std::size_t> Assign(const std::vector<Point>& units, const std::vector<Point>& slots) {
        if (units.size() != slots.size()) return {};
        std::vector<std::size_t> result;
        std::vector<bool> used(slots.size(),false);
        for (const Point& unit : units) {
            double best = (std::numeric_limits<double>::max)();
            std::size_t chosen = 0;
            for (std::size_t j=0; j<slots.size(); ++j) {
                const double cost = DistanceSquared(unit,slots[j]);
                if (!used[j] && cost < best) { best=cost; chosen=j; }
            }
            used[chosen]=true;
            result.push_back(chosen);
        }
        for (int pass=0; pass<2; ++pass) {
            bool changed=false;
            for (std::size_t i=0; i<units.size(); ++i) for (std::size_t j=i+1; j<units.size(); ++j) {
                const double oldCost=DistanceSquared(units[i],slots[result[i]])+DistanceSquared(units[j],slots[result[j]]);
                const double newCost=DistanceSquared(units[i],slots[result[j]])+DistanceSquared(units[j],slots[result[i]]);
                if (newCost+0.01 < oldCost) {
                    const std::size_t tmp=result[i]; result[i]=result[j]; result[j]=tmp; changed=true;
                }
            }
            if (!changed) break;
        }
        return result;
    }
}
