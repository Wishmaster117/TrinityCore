#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_PALADIN_PALADINCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_PALADIN_PALADINCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class PaladinCombatRotation final : public ICombatRotation
    {
    public:
        static PaladinCombatRotation& Instance()
        {
            static PaladinCombatRotation s;
            return s;
        }

        char const* Name() const override { return "PaladinCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_PALADIN_PALADINCOMBATROTATION_H