#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_PRIEST_PRIESTCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_PRIEST_PRIESTCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class PriestCombatRotation final : public ICombatRotation
    {
    public:
        static PriestCombatRotation& Instance()
        {
            static PriestCombatRotation s;
            return s;
        }

        char const* Name() const override { return "PriestCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_PRIEST_PRIESTCOMBATROTATION_H