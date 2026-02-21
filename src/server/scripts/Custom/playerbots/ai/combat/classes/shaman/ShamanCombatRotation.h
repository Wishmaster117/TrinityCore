#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_SHAMAN_SHAMANCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_SHAMAN_SHAMANCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat::Rotations
{
    class ShamanCombatRotation final : public ICombatRotation
    {
    public:
        static ShamanCombatRotation& Instance()
        {
            static ShamanCombatRotation s;
            return s;
        }

        char const* Name() const override { return "ShamanCombatRotation"; }

        uint32 SelectSpell(RotationContext const& /*ctx*/) override
        {
            return 0;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_SHAMAN_SHAMANCOMBATROTATION_H