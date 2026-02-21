#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_HUNTER_HUNTERNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_HUNTER_HUNTERNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class HunterNonCombatRotation final : public INonCombatRotation
    {
    public:
        static HunterNonCombatRotation& Instance()
        {
            static HunterNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "HunterNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_HUNTER_HUNTERNONCOMBATROTATION_H