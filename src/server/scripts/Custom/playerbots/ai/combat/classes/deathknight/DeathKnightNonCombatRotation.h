#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_DEATHKNIGHT_DEATHKNIGHTNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_DEATHKNIGHT_DEATHKNIGHTNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class DeathKnightNonCombatRotation final : public INonCombatRotation
    {
    public:
        static DeathKnightNonCombatRotation& Instance()
        {
            static DeathKnightNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "DeathKnightNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_DEATHKNIGHT_DEATHKNIGHTNONCOMBATROTATION_H