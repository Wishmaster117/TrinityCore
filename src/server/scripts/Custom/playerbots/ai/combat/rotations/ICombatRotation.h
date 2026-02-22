#ifndef PLAYERBOTS_AI_COMBAT_ROTATIONS_ICOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_ROTATIONS_ICOMBATROTATION_H

#pragma once
#include <cstdint>
#include "Define.h"

namespace Playerbots::AI::Combat::Rotations
{
    struct RotationContext;

    // Combat rotation returns:
    // - spellId to cast (base id, engine will resolve highest known rank)
    // - 0 means "no opinion" => engine fallback (generic spellbook)
    class ICombatRotation
    {
    public:
        virtual ~ICombatRotation() = default;
        virtual char const* Name() const = 0;
        virtual uint32 SelectSpell(RotationContext const& ctx) = 0;

        // Optional: AOE selection hook, called by BotCombatEngine during the dedicated AOE phase.
        // Default returns 0 to let the engine fallback to generic AOE bucket.
        virtual uint32 SelectAoeSpell(RotationContext const& /*ctx*/) { return 0; }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_ROTATIONS_ICOMBATROTATION_H