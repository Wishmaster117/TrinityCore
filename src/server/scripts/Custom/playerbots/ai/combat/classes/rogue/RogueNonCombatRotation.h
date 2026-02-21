#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_ROGUE_ROGUENONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_ROGUE_ROGUENONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class RogueNonCombatRotation final : public INonCombatRotation
    {
    public:
        static RogueNonCombatRotation& Instance()
        {
            static RogueNonCombatRotation s;
            return s;
        }

        char const* Name() const override { return "RogueNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_ROGUE_ROGUENONCOMBATROTATION_H