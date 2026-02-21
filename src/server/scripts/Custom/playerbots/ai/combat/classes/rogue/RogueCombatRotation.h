#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_ROGUE_ROGUECOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_ROGUE_ROGUECOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class RogueCombatRotation final : public ICombatRotation
    {
    public:
        static RogueCombatRotation& Instance()
        {
            static RogueCombatRotation s;
            return s;
        }

        char const* Name() const override { return "RogueCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_ROGUE_ROGUECOMBATROTATION_H