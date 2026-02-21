#ifndef PLAYERBOTS_AI_COMBAT_ROTATIONS_CLASSES_WARRIORCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_ROTATIONS_CLASSES_WARRIORCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    // Phase 3.3: spec coverage will be implemented incrementally.
    // For now, return 0 => engine fallback (generic spellbook categories).
    class WarriorCombatRotation final : public ICombatRotation
    {
    public:
        static WarriorCombatRotation& Instance()
        {
            static WarriorCombatRotation s;
            return s;
        }

        char const* Name() const override { return "WarriorCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_ROTATIONS_CLASSES_WARRIORCOMBATROTATION_H