#ifndef PLAYERBOTS_AI_COMBAT_NONCOMBAT_NONCOMBATBUFFHELPER_H
#define PLAYERBOTS_AI_COMBAT_NONCOMBAT_NONCOMBATBUFFHELPER_H

#include "Timer.h"
#include "Player.h"
#include "Unit.h"

#include <array>
#include <cstdint>

namespace Playerbots::AI::Combat::NonCombat
{
    // Simple bounded cooldown (ring) to prevent out-of-combat buff spam.
    // Keyed by (botGuidLow, targetGuidLow).
    struct BuffCooldownSlot
    {
        uint32 BotLow = 0;
        uint32 TargetLow = 0;
        uint32 UntilMs = 0;
    };

    static constexpr uint32 kBuffCooldownSlots = 128;

    inline std::array<BuffCooldownSlot, kBuffCooldownSlots>& BuffCooldownRing()
    {
        static std::array<BuffCooldownSlot, kBuffCooldownSlots> s_ring{};
        return s_ring;
    }

    inline uint32& BuffCooldownCursor()
    {
        static uint32 s_cursor = 0;
        return s_cursor;
    }

    inline bool IsOnBuffCooldown(Player* bot, Unit* target, uint32 nowMs)
    {
        if (!bot || !target)
            return true;

        uint32 b = bot->GetGUID().GetCounter();
        uint32 t = target->GetGUID().GetCounter();

        for (BuffCooldownSlot& s : BuffCooldownRing())
        {
            if (!s.BotLow)
                continue;

            if (s.UntilMs <= nowMs)
            {
                s.BotLow = 0;
                s.TargetLow = 0;
                s.UntilMs = 0;
                continue;
            }

            if (s.BotLow == b && s.TargetLow == t)
                return true;
        }

        return false;
    }

    inline void SetBuffCooldown(Player* bot, Unit* target, uint32 nowMs, uint32 delayMs)
    {
        if (!bot || !target)
            return;

        uint32 b = bot->GetGUID().GetCounter();
        uint32 t = target->GetGUID().GetCounter();

        // Update existing slot first.
        for (BuffCooldownSlot& s : BuffCooldownRing())
        {
            if (s.BotLow == b && s.TargetLow == t)
            {
                s.UntilMs = nowMs + delayMs;
                return;
            }
        }

        BuffCooldownSlot& s = BuffCooldownRing()[BuffCooldownCursor() % kBuffCooldownSlots];
        BuffCooldownCursor() = (BuffCooldownCursor() + 1) % kBuffCooldownSlots;
        s.BotLow = b;
        s.TargetLow = t;
        s.UntilMs = nowMs + delayMs;
    }
}

#endif // PLAYERBOTS_AI_COMBAT_NONCOMBAT_NONCOMBATBUFFHELPER_H