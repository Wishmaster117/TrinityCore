#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_DRUID_DRUIDNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_DRUID_DRUIDNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class DruidNonCombatRotation final : public INonCombatRotation
    {
    public:
        static DruidNonCombatRotation& Instance()
        {
            static DruidNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "DruidNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_DRUID_DRUIDNONCOMBATROTATION_H