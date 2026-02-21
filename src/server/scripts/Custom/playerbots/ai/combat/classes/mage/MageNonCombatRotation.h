#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_MAGE_MAGENONCOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_MAGE_MAGENONCOMBATROTATION_H

#include "playerbots/ai/combat/rotations/INonCombatRotation.h"

namespace Playerbots::AI::Combat::Rotations
{
    struct RotationContext;

    class MageNonCombatRotation final : public INonCombatRotation
    {
    public:
        static MageNonCombatRotation& Instance();

        char const* Name() const override { return "MageNonCombatRotation"; }
        uint32 SelectSpell(RotationContext const& ctx) override;

    private:
        MageNonCombatRotation() = default;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_MAGE_MAGENONCOMBATROTATION_H