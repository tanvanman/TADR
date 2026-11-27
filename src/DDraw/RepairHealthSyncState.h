#pragma once

#include <climits>
#include <cmath>
#include <cstdint>

namespace RepairHealthSyncState
{
    constexpr std::uint32_t kSampleMs = 200;
    constexpr std::uint32_t kPacketSpacingMs = 50;
    constexpr std::uint32_t kRecipientSpacingMs = 200;
    constexpr std::uint32_t kFlushIdleMs = 400;
    constexpr std::uint32_t kIdleReconcileDamageQuietMs = 1000;
    constexpr int kIdleReconcileDamageQuietTicks = 30;
    constexpr unsigned kCandidateChecksPerPass = 64;

    // Unsigned subtraction also handles GetTickCount's 49-day wrap.
    struct Cadence
    {
        std::uint32_t last = 0;
        bool started = false;

        bool Due(std::uint32_t now, std::uint32_t interval) const
        {
            return !started || now - last >= interval;
        }

        void Advance(std::uint32_t now)
        {
            last = now;
            started = true;
        }
    };

    inline unsigned short ClampHealth(int value, unsigned int maxHealth)
    {
        if (value <= 0 || maxHealth == 0)
            return 0;
        const unsigned int clampedMax = maxHealth > static_cast<unsigned int>(SHRT_MAX)
            ? static_cast<unsigned int>(SHRT_MAX)
            : maxHealth;
        if (static_cast<unsigned int>(value) >= clampedMax)
            return static_cast<unsigned short>(clampedMax);
        return static_cast<unsigned short>(value);
    }

    inline short ClampSignedDelta(int value)
    {
        if (value > SHRT_MAX)
            return SHRT_MAX;
        if (value < -SHRT_MAX)
            return static_cast<short>(-SHRT_MAX);
        return static_cast<short>(value);
    }

    inline int HealthStepThreshold(unsigned int maxHealth)
    {
        const unsigned int onePercent = maxHealth / 100u;
        return onePercent > 1u ? static_cast<int>(onePercent) : 1;
    }

    // Computes the unclamped post-completion HP so any in-flight Phase 1 repair
    // delta can still combine commutatively even after heavy damage during build.
    // Display paths clamp to [0, maxHealth] only when drawing or checking smoke.
    inline int ComputeCompletedRawHealth(int stockHealth, float remainingNanoframe,
        int unclampedClampLoss, unsigned int maxHealth)
    {
        if (maxHealth == 0)
            return 0;
        const unsigned int clampedMax = maxHealth > static_cast<unsigned int>(SHRT_MAX)
            ? static_cast<unsigned int>(SHRT_MAX)
            : maxHealth;
        const int clampedStock = stockHealth > 0 ? stockHealth : 0;
        float clampedNano = remainingNanoframe;
        if (clampedNano < 0.0f)
            clampedNano = 0.0f;
        else if (clampedNano > 1.0f)
            clampedNano = 1.0f;
        const int buildGain = static_cast<int>(
            std::lround(clampedNano * static_cast<float>(clampedMax)));
        const int clampedLoss = unclampedClampLoss > 0 ? unclampedClampLoss : 0;
        int result = clampedStock + buildGain - clampedLoss;
        if (static_cast<unsigned int>(result) > clampedMax && result > 0)
            result = static_cast<int>(clampedMax);
        return result;
    }

    inline bool ShouldFlushDelta(int pendingDelta, unsigned int maxHealth,
        bool reachedFullHealth, bool idleSettled)
    {
        if (pendingDelta == 0)
            return false;
        const int magnitude = pendingDelta < 0 ? -pendingDelta : pendingDelta;
        return magnitude >= HealthStepThreshold(maxHealth)
            || reachedFullHealth
            || idleSettled;
    }

    inline bool IsTickAtOrBefore(std::uint32_t candidateTick, std::uint32_t referenceTick)
    {
        return static_cast<std::int32_t>(candidateTick - referenceTick) <= 0;
    }

    inline bool IsInRecentCombat(bool hasLastDamage, bool playingDemo,
        std::uint32_t nowMs, std::uint32_t lastDamageMs,
        int gameTime, int lastDamageTick)
    {
        if (!hasLastDamage)
            return false;
        if (playingDemo)
        {
            return gameTime >= lastDamageTick
                && gameTime - lastDamageTick < kIdleReconcileDamageQuietTicks;
        }
        return nowMs - lastDamageMs < kIdleReconcileDamageQuietMs;
    }
}
