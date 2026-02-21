#ifndef PLAYERBOTS_AI_COMBAT_CLASSES_MAGE_MAGECOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_CLASSES_MAGE_MAGECOMBATROTATION_H

#include "playerbots/ai/combat/rotations/ICombatRotation.h"

namespace Playerbots::AI::Combat::Rotations
{
    struct RotationContext;

    // Phase 3.3 Mage (generic baseline):
    // - Keep it conservative and core-friendly: only pick from known spellbook via ids we verify at cast time.
    // - No specialization binding yet (Arcane/Fire/Frost will be added once spec API is confirmed).
    class MageCombatRotation final : public ICombatRotation
    {
    public:
        static MageCombatRotation& Instance();

        char const* Name() const override { return "MageCombatRotation"; }
        uint32 SelectSpell(RotationContext const& ctx) override;

    private:
        MageCombatRotation() = default;
        uint32 SelectGeneric(RotationContext const& ctx) const;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_CLASSES_MAGE_MAGECOMBATROTATION_H