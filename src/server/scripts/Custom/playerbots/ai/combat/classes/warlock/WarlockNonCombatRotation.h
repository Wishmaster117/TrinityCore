#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKNONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKNONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"

namespace Playerbots::AI::Combat::Rotations
{
    struct RotationContext;

    class WarlockNonCombatRotation final : public INonCombatRotation
    {
    public:
        static WarlockNonCombatRotation& Instance();

        char const* Name() const override { return "WarlockNonCombatRotation"; }

        uint32 SelectSpell(RotationContext const& ctx) override;

    private:
        WarlockNonCombatRotation() = default;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKNONCOMBATROTATION_H