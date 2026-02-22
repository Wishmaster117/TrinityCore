#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_HUNTER_HUNTERNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_HUNTER_HUNTERNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"

namespace Playerbots::AI::Combat::Rotations
{
    struct RotationContext;

    class HunterNonCombatRotation final : public INonCombatRotation
    {
    public:
        static HunterNonCombatRotation& Instance();

        char const* Name() const override { return "HunterNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& ctx) override;

    private:
        HunterNonCombatRotation() = default;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_HUNTER_HUNTERNONCOMBATROTATION_H