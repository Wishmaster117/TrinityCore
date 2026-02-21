#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class WarlockCombatRotation final : public ICombatRotation
    {
    public:
        static WarlockCombatRotation& Instance()
        {
            static WarlockCombatRotation s;
            return s;
        }

        char const* Name() const override { return "WarlockCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKCOMBATROTATION_H