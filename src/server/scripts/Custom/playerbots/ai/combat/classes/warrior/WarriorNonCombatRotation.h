#ifndef PLAYERBOTS_AI_COMBAT_ROTATIONS_CLASSES_WARRIORNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_ROTATIONS_CLASSES_WARRIORNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class WarriorNonCombatRotation final : public INonCombatRotation
    {
    public:
        static WarriorNonCombatRotation& Instance()
        {
            static WarriorNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "WarriorNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_ROTATIONS_CLASSES_WARRIORNONCOMBATROTATION_H