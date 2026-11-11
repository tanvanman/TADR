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
    // Sweep complete horizontal rows from the farther Y edge toward the builder.
    // Alternate X direction each row, ending at the nearer X corner. Midpoint
    // ties finish at the top row and the left corner.
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
        const bool finishAtTop = Coordinate(builderY) * 2 <= top + bottom;
        const bool finalIncreasing = Coordinate(builderX) * 2 > left + right;
        // Choose the first X direction from the row parity so the final row
        // ends closest to the builder, for either vertical sweep direction.
        bool increasing = finalIncreasing != ((rows - 1) % 2 != 0);
        const Coordinate firstY = finishAtTop ? bottom : top;
        const Coordinate incrementY = finishAtTop ? -stepY : stepY;

        points.reserve(static_cast<std::size_t>(columns * rows));
        for (Coordinate row = 0; row < rows; ++row)
        {
            const Coordinate y = firstY + row * incrementY;
            const Coordinate firstX = increasing ? left : right;
            const Coordinate incrementX = increasing ? stepX : -stepX;
            for (Coordinate column = 0; column < columns; ++column)
            {
                const Coordinate x = firstX + column * incrementX;
                points.push_back({static_cast<int>(x), static_cast<int>(y)});
            }
            increasing = !increasing;
        }
        return true;
    }
}
