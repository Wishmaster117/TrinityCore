#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_EVOKER_EVOKERCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_EVOKER_EVOKERCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class EvokerCombatRotation final : public ICombatRotation
    {
    public:
        static EvokerCombatRotation& Instance()
        {
            static EvokerCombatRotation s;
            return s;
        }

        char const* Name() const override { return "EvokerCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_EVOKER_EVOKERCOMBATROTATION_H