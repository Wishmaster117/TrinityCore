#include "playerbots/ai/combat/classes/warlock/WarlockCombatRotation.h"

#include "playerbots/ai/combat/rotations/RotationContext.h"
#include "playerbots/ai/combat/SpellCatalog.h"

#include "Config.h"
#include "Player.h"
#include "SpellDefines.h"
#include "Unit.h"

namespace Playerbots::AI::Combat::Rotations
{
    namespace
    {
        static bool AllowAnySchoolFallback()
        {
            return sConfigMgr->GetBoolDefault("Playerbots.Rotations.Warlock.AllowAnySchoolFallback", true);
        }

        static SpellSchoolMask SpecToSchoolMask(Player* bot)
        {
            if (!bot)
                return SPELL_SCHOOL_MASK_NONE;

            // Warlock specs (OrderIndex is stable per class in DB2):
            // 0 = Affliction, 1 = Demonology, 2 = Destruction
            // Heuristic school mapping:
            // - Affliction: Shadow
            // - Demonology: Shadow (most baseline nukes/dots remain shadow)
            // - Destruction: Fire
            if (ChrSpecializationEntry const* entry = bot->GetPrimarySpecializationEntry())
            {
                switch (entry->OrderIndex)
                {
                    case 0: return SPELL_SCHOOL_MASK_SHADOW;
                    case 1: return SPELL_SCHOOL_MASK_SHADOW;
                    case 2: return SPELL_SCHOOL_MASK_FIRE;
                    default: break;
                }
            }
            return SPELL_SCHOOL_MASK_NONE;
        }

        static uint32 SelectSchoolRotation(RotationContext const& ctx, SpellSchoolMask mask)
        {
            if (!ctx.Bot || !ctx.Victim || !ctx.Catalog)
                return 0;

            // Warlock priority (MVP, human-like):
            // 1) Maintain a safe DoT if missing.
            if (uint32 dot = ctx.Catalog->BestSingleTargetDotBySchool(ctx.Bot, mask))
            {
                if (!ctx.Catalog->TargetHasAuraFromSpellChain(ctx.Victim, dot))
                    return dot;
            }

            // 2) Instant nuke (safe, no cooldown, no channel).
            if (uint32 s = ctx.Catalog->BestInstantNukeBySchool(ctx.Bot, mask))
                return s;

            // 3) Core nuke (cast-time preferred).
            if (uint32 n = ctx.Catalog->BestSingleTargetNukeBySchool(ctx.Bot, mask))
                return n;

            return 0;
        }
    }

    WarlockCombatRotation& WarlockCombatRotation::Instance()
    {
        static WarlockCombatRotation s;
        return s;
    }

    uint32 WarlockCombatRotation::SelectSpell(RotationContext const& ctx)
    {
        if (!ctx.Bot || !ctx.Victim || !ctx.Catalog)
            return 0;

        SpellSchoolMask mask = SpecToSchoolMask(ctx.Bot);
        if (mask != SPELL_SCHOOL_MASK_NONE)
        {
            if (uint32 s = SelectSchoolRotation(ctx, mask))
                return s;
        }

        // Fallback: any-school safe rotation (still spellbook-driven).
        if (AllowAnySchoolFallback())
            if (uint32 s = SelectSchoolRotation(ctx, SPELL_SCHOOL_MASK_NONE))
                return s;

        return 0;
    }

    uint32 WarlockCombatRotation::SelectAoeSpell(RotationContext const& ctx)
    {
        if (!ctx.Bot || !ctx.Victim || !ctx.Catalog)
            return 0;

        if (!ctx.Catalog->HasAoe())
            return 0;

        if (ctx.AoeMinTargets == 0 || ctx.NearbyEnemies < ctx.AoeMinTargets)
            return 0;

        SpellSchoolMask mask = SpecToSchoolMask(ctx.Bot);
        if (mask != SPELL_SCHOOL_MASK_NONE)
            if (uint32 s = ctx.Catalog->BestAoeBySchool(ctx.Bot, mask))
                return s;

        return ctx.Catalog->BestAoeBySchool(ctx.Bot, SPELL_SCHOOL_MASK_NONE);
    }
}