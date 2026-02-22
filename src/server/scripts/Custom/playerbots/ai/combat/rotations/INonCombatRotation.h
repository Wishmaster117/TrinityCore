#ifndef PLAYERBOTS_AI_COMBAT_ROTATIONS_INONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_ROTATIONS_INONCOMBATROTATION_H

#pragma once
#include <cstdint>
#include "Define.h"

namespace Playerbots::AI::Combat::Rotations
{
    struct RotationContext;

    // Non-combat rotation returns spellId to cast (buff, regen, etc.), or 0 if none.
    class INonCombatRotation
    {
    public:
        virtual ~INonCombatRotation() = default;
        virtual char const* Name() const = 0;
        virtual uint32 SelectSpell(RotationContext const& ctx) = 0;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_ROTATIONS_INONCOMBATROTATION_H