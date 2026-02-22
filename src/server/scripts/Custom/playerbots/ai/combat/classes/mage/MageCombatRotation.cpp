#include "playerbots/ai/combat/classes/mage/MageCombatRotation.h"

#include "playerbots/ai/combat/rotations/RotationContext.h"
#include "playerbots/ai/combat/SpellCatalog.h"

#include "Config.h"
#include "Player.h"
#include "SharedDefines.h"
#include "Unit.h"

namespace Playerbots::AI::Combat::Rotations
{
    namespace
    {
        static bool AllowAnySchoolFallback()
        {
            // Conservative default: enabled, because it prevents "no-op" rotations when the chosen school has no safe filler.
            // If you prefer strict behavior, set it to 0 in your config.
            return sConfigMgr->GetBoolDefault("Playerbots.Rotations.Mage.AllowAnySchoolFallback", true);
        }

        static uint32 SelectSchoolRotation(RotationContext const& ctx, SpellSchoolMask mask)
        {
            if (!ctx.Bot || !ctx.Victim || !ctx.Catalog)
                return 0;

            // 1) Instant nuke (safe, no cooldown, no channel).
            if (uint32 s = ctx.Catalog->BestInstantNukeBySchool(ctx.Bot, mask))
                return s;

            // 2) DoT maintenance (safe periodic damage) if not already present (spell chain aware).
            if (uint32 dot = ctx.Catalog->BestSingleTargetDotBySchool(ctx.Bot, mask))
            {
                if (!ctx.Catalog->TargetHasAuraFromSpellChain(ctx.Victim, dot))
                    return dot;
            }

            // 3) Core nuke (cast-time preferred).
            if (uint32 n = ctx.Catalog->BestSingleTargetNukeBySchool(ctx.Bot, mask))
                return n;

            return 0;
        }

        static uint32 SelectArcane(RotationContext const& ctx)
        {
            if (!ctx.Bot || !ctx.Catalog)
                return 0;

            if (uint32 s = SelectSchoolRotation(ctx, SPELL_SCHOOL_MASK_ARCANE))
                return s;

            // 3.3.4: optional fallback to any-school safe filler.
            if (AllowAnySchoolFallback())
                return SelectSchoolRotation(ctx, SPELL_SCHOOL_MASK_NONE);

            return 0;
        }

        static uint32 SelectFire(RotationContext const& ctx)
        {
            if (!ctx.Bot || !ctx.Catalog)
                return 0;

            if (uint32 s = SelectSchoolRotation(ctx, SPELL_SCHOOL_MASK_FIRE))
                return s;

            if (AllowAnySchoolFallback())
                return SelectSchoolRotation(ctx, SPELL_SCHOOL_MASK_NONE);

            return 0;
        }

        static uint32 SelectFrost(RotationContext const& ctx)
        {
            if (!ctx.Bot || !ctx.Catalog)
                return 0;

            if (uint32 s = SelectSchoolRotation(ctx, SPELL_SCHOOL_MASK_FROST))
                return s;

            if (AllowAnySchoolFallback())
                return SelectSchoolRotation(ctx, SPELL_SCHOOL_MASK_NONE);

            return 0;
        }
    }

    MageCombatRotation& MageCombatRotation::Instance()
    {
        static MageCombatRotation s;
        return s;
    }

    uint32 MageCombatRotation::SelectAoeSpell(RotationContext const& ctx)
    {
        if (!ctx.Bot || !ctx.Victim || !ctx.Catalog)
            return 0;

        if (!ctx.Catalog->HasAoe())
            return 0;

        if (ctx.AoeMinTargets == 0 || ctx.NearbyEnemies < ctx.AoeMinTargets)
            return 0;

        // Spec-aware AOE selection (spellbook-driven, no IDs).
        // Prefer school matching spec when possible; fallback to any-school AOE.
        SpellSchoolMask mask = SPELL_SCHOOL_MASK_NONE;
        if (ChrSpecializationEntry const* entry = ctx.Bot->GetPrimarySpecializationEntry())
        {
            switch (entry->OrderIndex)
            {
                case 0: mask = SPELL_SCHOOL_MASK_ARCANE; break;
                case 1: mask = SPELL_SCHOOL_MASK_FIRE; break;
                case 2: mask = SPELL_SCHOOL_MASK_FROST; break;
                default: break;
            }
        }

        if (mask != SPELL_SCHOOL_MASK_NONE)
            if (uint32 s = ctx.Catalog->BestAoeBySchool(ctx.Bot, mask))
                return s;

        return ctx.Catalog->BestAoeBySchool(ctx.Bot, SPELL_SCHOOL_MASK_NONE);
    }

    uint32 MageCombatRotation::SelectSpell(RotationContext const& ctx)
    {
        if (!ctx.Bot || !ctx.Victim)
            return 0;

        // Prefer using the entry when available (future-proof) — we don't rely on numeric IDs here.
        if (ChrSpecializationEntry const* entry = ctx.Bot->GetPrimarySpecializationEntry())
        {
            // OrderIndex is stable per class (0..2 for most classes).
            switch (entry->OrderIndex)
            {
                case 0: // Arcane (Mage)
                    if (uint32 s = SelectArcane(ctx))
                        return s;
                    break;
                case 1: // Fire (Mage)
                    if (uint32 s = SelectFire(ctx))
                        return s;
                    break;
                case 2: // Frost (Mage)
                    if (uint32 s = SelectFrost(ctx))
                        return s;
                    break;
                default:
                    break;
            }
        }

        return SelectGeneric(ctx);;
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
        // Next step (Mage 3.3.2):
        // - return candidate ids for core spells if known (Arcane Blast / Fireball / Frostbolt),
        //   using SpellCatalog utilities to check presence.
        return 0;
    }
}