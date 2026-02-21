#include "playerbots/ai/combat/classes/mage/MageCombatRotation.h"

#include "playerbots/ai/combat/rotations/RotationContext.h"

#include "Player.h"
#include "Unit.h"

namespace Playerbots::AI::Combat::Rotations
{
    MageCombatRotation& MageCombatRotation::Instance()
    {
        static MageCombatRotation s;
        return s;
    }

    uint32 MageCombatRotation::SelectSpell(RotationContext const& ctx)
    {
        return SelectGeneric(ctx);
    }

    uint32 MageCombatRotation::SelectGeneric(RotationContext const& ctx) const
    {
        if (!ctx.Bot || !ctx.Victim)
            return 0;

        // Simple, robust heuristics:
        // - If victim is far, we prefer a ranged nuke (engine will verify and resolve rank).
        // - If mana is low, avoid suggesting extra spells and let engine fallback.
        if (ctx.Bot->GetPowerType() == POWER_MANA && ctx.Bot->GetPowerPct(POWER_MANA) < 10.0f)
            return 0;

        // We intentionally return 0 for now to use the generic engine spellbook selection
        // (interrupt/dispel/buff/aoe/heal + offensive). This keeps behavior correct across expansions.
        //
        // Next step (Mage 3.3.1):
        // - return candidate ids for core spells if known (Arcane Blast / Fireball / Frostbolt),
        //   using SpellCatalog utilities to check presence.
        return 0;
    }
}