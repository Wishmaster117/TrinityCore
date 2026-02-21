#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_DEATHKNIGHT_DEATHKNIGHTCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_DEATHKNIGHT_DEATHKNIGHTCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class DeathKnightCombatRotation final : public ICombatRotation
    {
    public:
        static DeathKnightCombatRotation& Instance()
        {
            static DeathKnightCombatRotation s;
            return s;
        }

        char const* Name() const override { return "DeathKnightCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            // Phase 3.3: implement per-spec rotation later.
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_DEATHKNIGHT_DEATHKNIGHTCOMBATROTATION_H