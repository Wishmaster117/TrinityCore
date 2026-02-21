#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_DEMONHUNTER_DEMONHUNTERNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_DEMONHUNTER_DEMONHUNTERNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class DemonHunterNonCombatRotation final : public INonCombatRotation
    {
    public:
        static DemonHunterNonCombatRotation& Instance()
        {
            static DemonHunterNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "DemonHunterNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_DEMONHUNTER_DEMONHUNTERNONCOMBATROTATION_H