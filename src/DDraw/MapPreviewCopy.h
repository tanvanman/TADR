#pragma once

#include <cstddef>
#include <cstring>

// TNT minimaps contain opaque palette indices. No byte value is a color key.
// This helper copies only the temporary frame in TA's existing preview scaler.
namespace MapPreviewCopy
{
    inline bool CopyOpaque(unsigned char* destination, int destinationWidth,
        int destinationHeight, int destinationPitch, const unsigned char* source,
        int sourceWidth, int sourceHeight)
    {
        if (!destination || !source || sourceWidth <= 0 || sourceHeight <= 0 ||
            destinationWidth != sourceWidth || destinationHeight != sourceHeight ||
            destinationPitch < sourceWidth)
            return false;

        for (int y = 0; y < sourceHeight; ++y)
            std::memcpy(destination + static_cast<std::size_t>(y) * destinationPitch,
                source + static_cast<std::size_t>(y) * sourceWidth, sourceWidth);
        return true;
    }
}
