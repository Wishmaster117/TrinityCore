#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_MONK_MONKNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_MONK_MONKNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class MonkNonCombatRotation final : public INonCombatRotation
    {
    public:
        static MonkNonCombatRotation& Instance()
        {
            static MonkNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "MonkNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_MONK_MONKNONCOMBATROTATION_H