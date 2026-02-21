#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_MONK_MONKCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_MONK_MONKCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class MonkCombatRotation final : public ICombatRotation
    {
    public:
        static MonkCombatRotation& Instance()
        {
            static MonkCombatRotation s;
            return s;
        }

        char const* Name() const override { return "MonkCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_MONK_MONKCOMBATROTATION_H