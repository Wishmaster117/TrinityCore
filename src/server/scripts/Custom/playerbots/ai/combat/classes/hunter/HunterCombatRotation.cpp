#include "playerbots/ai/combat/classes/hunter/HunterCombatRotation.h"

#include "playerbots/ai/combat/rotations/RotationContext.h"
#include "playerbots/ai/combat/SpellCatalog.h"

#include "Config.h"
#include "Player.h"
#include "Pet.h"
#include "PetDefines.h"
#include "MotionMaster.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Unit.h"

#include <unordered_map>

namespace Playerbots::AI::Combat::Rotations
{
    namespace
    {
        static bool AllowAnySchoolFallback()
        {
            return sConfigMgr->GetBoolDefault("Playerbots.Rotations.Hunter.AllowAnySchoolFallback", true);
        }

        // Throttle pet command spam (attack/follow).
        static bool CanIssuePetCommandNow(Player* bot, uint32 nowMs, uint32 throttleMs)
        {
            static std::unordered_map<ObjectGuid, uint32> s_nextAllowedMs;
            if (!bot)
                return false;

            ObjectGuid guid = bot->GetGUID();
            auto it = s_nextAllowedMs.find(guid);
            if (it != s_nextAllowedMs.end() && nowMs < it->second)
                return false;

            s_nextAllowedMs[guid] = nowMs + throttleMs;
            return true;
        }

        static bool SpellHasEffect(SpellInfo const* info, uint32 effect)
        {
            if (!info)
                return false;

            for (SpellEffectInfo const& eff : info->GetEffects())
                if (static_cast<uint32>(eff.Effect) == effect)
                    return true;

            return false;
        }

        // Find the "best" known spell that has a specific SpellEffect.
        // Heuristic: prefer lower cast time, then higher spell level, then stable higher id.
        static uint32 FindBestKnownSpellWithEffect(Player* bot, uint32 effect)
        {
            if (!bot || !bot->GetMap())
                return 0;

            Difficulty diff = Difficulty(bot->GetMap()->GetDifficultyID());

            uint32 bestId = 0;
            uint32 bestCast = 0;
            uint32 bestLevel = 0;

            for (auto const& [spellId, data] : bot->GetSpellMap())
            {
                if (data.state == PLAYERSPELL_REMOVED || data.disabled)
                    continue;

                SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, diff);
                if (!info)
                    continue;

                if (info->IsPassive())
                    continue;

                if (!SpellHasEffect(info, effect))
                    continue;

                uint32 castMs = info->CalcCastTime();
                uint32 lvl = info->SpellLevel;

                if (!bestId)
                {
                    bestId = spellId;
                    bestCast = castMs;
                    bestLevel = lvl;
                    continue;
                }

                if (castMs != bestCast)
                {
                    if (castMs < bestCast)
                    {
                        bestId = spellId;
                        bestCast = castMs;
                        bestLevel = lvl;
                    }
                    continue;
                }

                if (lvl != bestLevel)
                {
                    if (lvl > bestLevel)
                    {
                        bestId = spellId;
                        bestCast = castMs;
                        bestLevel = lvl;
                    }
                    continue;
                }

                if (spellId > bestId)
                    bestId = spellId;
            }

            return bestId;
        }

        // "Human-like" pet management:
        // - If no pet => try Call/Summon Pet.
        // - If pet dead => try Revive Pet.
        // Returned spell is expected to be positive/self-cast; BotCombatEngine will self-cast positive spells.
        static uint32 EnsurePetSpell(Player* bot)
        {
            if (!bot)
                return 0;

            Pet* pet = bot->GetPet();
            if (!pet)
            {
                // Call Pet / Summon Pet is modeled by SPELL_EFFECT_SUMMON_PET.
                return FindBestKnownSpellWithEffect(bot, SPELL_EFFECT_SUMMON_PET);
            }

            if (!pet->IsAlive())
            {
                // Revive Pet is modeled by SPELL_EFFECT_RESURRECT_PET.
                return FindBestKnownSpellWithEffect(bot, SPELL_EFFECT_RESURRECT_PET);
            }

            return 0;
        }

        static void TryControlPet(Player* bot, Unit* victim)
        {
            if (!bot)
                return;

            Pet* pet = bot->GetPet();
            if (!pet || !pet->IsAlive())
                return;

            uint32 nowMs = getMSTime();
            if (!CanIssuePetCommandNow(bot, nowMs, 900))
                return;

            // If pet is very far, force follow to prevent "pet stuck" feeling.
            float distToOwner = pet->GetDistance(bot);
            if (distToOwner > 45.0f)
            {
                pet->GetMotionMaster()->MoveFollow(bot, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                return;
            }

            if (!victim || !victim->IsAlive())
                return;

            // If pet isn't currently attacking the target, send it in.
            Unit* petVictim = pet->GetVictim();
            if (petVictim != victim)
            {
                // Attack will engage and chase via pet AI.
                pet->Attack(victim, true);
                pet->GetMotionMaster()->MoveChase(victim);
            }
        }

        static uint32 SelectAnySchoolRotation(RotationContext const& ctx)
        {
            if (!ctx.Bot || !ctx.Victim || !ctx.Catalog)
                return 0;

            // Hunter (spellbook-driven, DPS-first):
            // 1) Maintain a DoT if we have one and it's missing.
            if (uint32 dot = ctx.Catalog->BestSingleTargetDotBySchool(ctx.Bot, SPELL_SCHOOL_MASK_NONE))
            {
                if (!ctx.Catalog->TargetHasAuraFromSpellChain(ctx.Victim, dot))
                    return dot;
            }

            // 2) Instant nuke (no cooldown, no channel).
            if (uint32 inst = ctx.Catalog->BestInstantNukeBySchool(ctx.Bot, SPELL_SCHOOL_MASK_NONE))
                return inst;

            // 3) Cast-time nuke filler.
            if (uint32 nuke = ctx.Catalog->BestSingleTargetNukeBySchool(ctx.Bot, SPELL_SCHOOL_MASK_NONE))
                return nuke;

            return 0;
        }
    }

    HunterCombatRotation& HunterCombatRotation::Instance()
    {
        static HunterCombatRotation s;
        return s;
    }

    uint32 HunterCombatRotation::SelectSpell(RotationContext const& ctx)
    {
        if (!ctx.Bot || !ctx.Catalog)
            return 0;

        // Pet management has priority, but do not hardcode SpellIDs:
        // discover Call/Summon/Revive from the spellbook via SpellEffects.
        if (uint32 petSpell = EnsurePetSpell(ctx.Bot))
            return petSpell;

        // Pet control is "side effect" (no casting) and is throttled.
        TryControlPet(ctx.Bot, ctx.Victim);

        if (!ctx.Victim || !ctx.Victim->IsAlive())
            return 0;

        // "Any school" primary rotation. Hunters have many physical spells; school mapping is unreliable here.
        if (uint32 s = SelectAnySchoolRotation(ctx))
            return s;

        // Optional: allow a wider fallback (still spellbook-driven).
        // Currently SelectAnySchoolRotation already uses NONE mask; keep the toggle for consistency with other classes.
        if (AllowAnySchoolFallback())
            return SelectAnySchoolRotation(ctx);

        return 0;
    }

    uint32 HunterCombatRotation::SelectAoeSpell(RotationContext const& ctx)
    {
        if (!ctx.Bot || !ctx.Victim || !ctx.Catalog)
            return 0;

        if (!ctx.Catalog->HasAoe())
            return 0;

        if (ctx.AoeMinTargets == 0 || ctx.NearbyEnemies < ctx.AoeMinTargets)
            return 0;

        return ctx.Catalog->BestAoeBySchool(ctx.Bot, SPELL_SCHOOL_MASK_NONE);
    }
}