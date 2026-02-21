#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_SHAMAN_SHAMANNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_SHAMAN_SHAMANNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class ShamanNonCombatRotation final : public INonCombatRotation
    {
    public:
        static ShamanNonCombatRotation& Instance()
        {
            static ShamanNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "ShamanNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_SHAMAN_SHAMANNONCOMBATROTATION_H