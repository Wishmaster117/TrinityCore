#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_DRUID_DRUIDCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_DRUID_DRUIDCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class DruidCombatRotation final : public ICombatRotation
    {
    public:
        static DruidCombatRotation& Instance()
        {
            static DruidCombatRotation s;
            return s;
        }

        char const* Name() const override { return "DruidCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_DRUID_DRUIDCOMBATROTATION_H