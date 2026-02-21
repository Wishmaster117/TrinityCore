#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_DEMONHUNTER_DEMONHUNTERCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_DEMONHUNTER_DEMONHUNTERCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class DemonHunterCombatRotation final : public ICombatRotation
    {
    public:
        static DemonHunterCombatRotation& Instance()
        {
            static DemonHunterCombatRotation s;
            return s;
        }

        char const* Name() const override { return "DemonHunterCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_DEMONHUNTER_DEMONHUNTERCOMBATROTATION_H