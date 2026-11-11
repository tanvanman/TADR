#pragma once

namespace BuildRectangleOrders
{
    // Queue ordinary per-constructor moves onto the unfinished side of a strip.
    // The target is a snapped building centre; outward is one cardinal unit vector.
    // A failed destination preflight queues nothing, so the caller can leave the
    // last strip open. Native pathfinding still decides whether a move succeeds.
    bool QueueRetreat(int targetX, int targetY, int outwardX, int outwardY,
        int buildingFootX, int buildingFootY, bool finishing);
}
