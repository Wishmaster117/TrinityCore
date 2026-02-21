#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class WarlockNonCombatRotation final : public INonCombatRotation
    {
    public:
        static WarlockNonCombatRotation& Instance()
        {
            static WarlockNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "WarlockNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKNONCOMBATROTATION_H