#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_PRIEST_PRIESTNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_PRIEST_PRIESTNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class PriestNonCombatRotation final : public INonCombatRotation
    {
    public:
        static PriestNonCombatRotation& Instance()
        {
            static PriestNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "PriestNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_PRIEST_PRIESTNONCOMBATROTATION_H