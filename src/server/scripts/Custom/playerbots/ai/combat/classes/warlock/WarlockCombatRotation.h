#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"

namespace Playerbots::AI::Combat::Rotations
{
    struct RotationContext;

    class WarlockCombatRotation final : public ICombatRotation
    {
    public:
        static WarlockCombatRotation& Instance();

        char const* Name() const override { return "WarlockCombatRotation"; }

        uint32 SelectSpell(RotationContext const& ctx) override;
        uint32 SelectAoeSpell(RotationContext const& ctx) override;

    private:
        WarlockCombatRotation() = default;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_WARLOCK_WARLOCKCOMBATROTATION_H