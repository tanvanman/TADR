#pragma once

#include <cstdint>
#include <vector>

namespace BuildRectangleGeometry
{
    struct Point
    {
        int x;
        int y;
    };

    // Coordinates are building centres on the world grid. Preserve the first
    // corner as the grid origin, including when the drag runs left or upward.
    // Sweep from the far side toward the builder, ending at its nearest corner.
    // The caller reserves its own sentinel slot outside this capacity.
    inline bool Generate(int startX, int startY, int endX, int endY,
        int footprintX, int footprintY, int spacing, int builderX, int builderY,
        int capacity, std::vector<Point>& points)
    {
        points.clear();
        if (footprintX <= 0 || footprintY <= 0 || spacing < 0 || capacity <= 0)
            return false;

        using Coordinate = std::int64_t;
        const Coordinate deltaX = Coordinate(endX) - startX;
        const Coordinate deltaY = Coordinate(endY) - startY;
        const Coordinate stepX = (Coordinate(footprintX) + spacing) * 16;
        const Coordinate stepY = (Coordinate(footprintY) + spacing) * 16;
        const Coordinate columns = (deltaX < 0 ? -deltaX : deltaX) / stepX + 1;
        const Coordinate rows = (deltaY < 0 ? -deltaY : deltaY) / stepY + 1;
        // Reject the entire rectangle, so previews and orders describe the
        // same complete shape. Division also avoids multiplying large counts.
        if (columns > capacity || rows > capacity || columns > capacity / rows)
            return false;

        const Coordinate lastX = Coordinate(startX) + (columns - 1) * (deltaX < 0 ? -stepX : stepX);
        const Coordinate lastY = Coordinate(startY) + (rows - 1) * (deltaY < 0 ? -stepY : stepY);
        const Coordinate left = startX < lastX ? startX : lastX;
        const Coordinate right = startX > lastX ? startX : lastX;
        const Coordinate top = startY < lastY ? startY : lastY;
        const Coordinate bottom = startY > lastY ? startY : lastY;
        const Coordinate clampedX = builderX < left ? left : builderX > right ? right : builderX;
        const Coordinate clampedY = builderY < top ? top : builderY > bottom ? bottom : builderY;
        const auto square = [](Coordinate value) -> long double {
            return static_cast<long double>(value) * value;
        };

        // Distance to each side segment also handles builders beyond a corner
        // and rectangles with a single row or column. Ties prefer left, right,
        // top, then bottom, making the order deterministic.
        enum Side { Left, Right, Top, Bottom };
        const long double distances[] = {
            square(Coordinate(builderX) - left) + square(Coordinate(builderY) - clampedY),
            square(Coordinate(builderX) - right) + square(Coordinate(builderY) - clampedY),
            square(Coordinate(builderX) - clampedX) + square(Coordinate(builderY) - top),
            square(Coordinate(builderX) - clampedX) + square(Coordinate(builderY) - bottom)
        };
        Side nearest = Left;
        for (int side = Right; side <= Bottom; ++side)
            if (distances[side] < distances[nearest]) nearest = static_cast<Side>(side);

        const bool verticalSide = nearest == Left || nearest == Right;
        const bool nearLowSide = nearest == Left || nearest == Top;
        const Coordinate layers = verticalSide ? columns : rows;
        const Coordinate cellsPerLayer = verticalSide ? rows : columns;
        const Coordinate outerLow = verticalSide ? left : top;
        const Coordinate outerHigh = verticalSide ? right : bottom;
        const Coordinate innerLow = verticalSide ? top : left;
        const Coordinate innerHigh = verticalSide ? bottom : right;
        const Coordinate outerStep = verticalSide ? stepX : stepY;
        const Coordinate innerStep = verticalSide ? stepY : stepX;
        const Coordinate builderInner = verticalSide ? builderY : builderX;
        const bool finalIncreasing = square(builderInner - innerHigh) < square(builderInner - innerLow);
        // Choose the initial direction from the layer parity, so serpentine
        // traversal finishes at the selected side's closest endpoint.
        bool increasing = finalIncreasing != ((layers - 1) % 2 != 0);
        const Coordinate firstOuter = nearLowSide ? outerHigh : outerLow;
        const Coordinate outerIncrement = nearLowSide ? -outerStep : outerStep;

        points.reserve(static_cast<std::size_t>(columns * rows));
        for (Coordinate layer = 0; layer < layers; ++layer)
        {
            const Coordinate outer = firstOuter + layer * outerIncrement;
            const Coordinate firstInner = increasing ? innerLow : innerHigh;
            const Coordinate innerIncrement = increasing ? innerStep : -innerStep;
            for (Coordinate cell = 0; cell < cellsPerLayer; ++cell)
            {
                const Coordinate inner = firstInner + cell * innerIncrement;
                const Point point = verticalSide
                    ? Point{static_cast<int>(outer), static_cast<int>(inner)}
                    : Point{static_cast<int>(inner), static_cast<int>(outer)};
                points.push_back(point);
            }
            increasing = !increasing;
        }
        return true;
    }
}
