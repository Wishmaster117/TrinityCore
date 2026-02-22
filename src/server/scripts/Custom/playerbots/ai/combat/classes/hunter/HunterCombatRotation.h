#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_HUNTER_HUNTERCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_HUNTER_HUNTERCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"

namespace Playerbots::AI::Combat::Rotations
{
    struct RotationContext;
	
    class HunterCombatRotation final : public ICombatRotation
    {
    public:
        static HunterCombatRotation& Instance();

        char const* Name() const override { return "HunterCombatRotation"; }

        uint32 SelectSpell(RotationContext const& ctx) override;
        uint32 SelectAoeSpell(RotationContext const& ctx) override;

    private:
        HunterCombatRotation() = default;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_HUNTER_HUNTERCOMBATROTATION_H