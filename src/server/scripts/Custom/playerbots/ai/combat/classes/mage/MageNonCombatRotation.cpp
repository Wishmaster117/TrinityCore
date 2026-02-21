#include "playerbots/ai/combat/classes/mage/MageNonCombatRotation.h"

#include "playerbots/ai/combat/rotations/RotationContext.h"

#include "Player.h"

namespace Playerbots::AI::Combat::Rotations
{
    MageNonCombatRotation& MageNonCombatRotation::Instance()
    {
        static MageNonCombatRotation s;
        return s;
    }

    uint32 MageNonCombatRotation::SelectSpell(RotationContext const& ctx)
    {
        if (!ctx.Bot)
            return 0;

        // Keep non-combat logic minimal for now.
        // Buffing is already handled by the generic engine; later we can specialize:
        // - Arcane Intellect if missing
        // - Conjure food/water if needed
        return 0;
    }
}