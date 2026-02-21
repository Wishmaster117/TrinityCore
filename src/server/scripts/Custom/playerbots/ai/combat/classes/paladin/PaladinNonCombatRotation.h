#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_PALADIN_PALADINNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_PALADIN_PALADINNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class PaladinNonCombatRotation final : public INonCombatRotation
    {
    public:
        static PaladinNonCombatRotation& Instance()
        {
            static PaladinNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "PaladinNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_PALADIN_PALADINNONCOMBATROTATION_H