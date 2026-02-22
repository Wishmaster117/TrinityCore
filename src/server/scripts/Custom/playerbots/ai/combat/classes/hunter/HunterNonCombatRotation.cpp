#include "playerbots/ai/combat/classes/hunter/HunterNonCombatRotation.h"

#include "playerbots/ai/combat/rotations/RotationContext.h"
#include "playerbots/ai/combat/SpellCatalog.h"
#include "playerbots/ai/combat/noncombat/EatDrinkHelper.h"
#include "playerbots/ai/combat/noncombat/NonCombatBuffHelper.h"

#include "Config.h"
#include "Player.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Timer.h"

#include <unordered_map>

namespace Playerbots::AI::Combat::Rotations
{
    namespace
    {
        static bool SpellHasEffect(SpellInfo const* info, uint32 effect)
        {
            if (!info)
                return false;

            for (SpellEffectInfo const& eff : info->GetEffects())
                if (static_cast<uint32>(eff.Effect) == effect)
                    return true;

            return false;
        }

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

        // Avoid spamming Call Pet while idle.
        static bool CanTryCallPetNow(Player* bot, uint32 nowMs)
        {
            static std::unordered_map<ObjectGuid, uint32> s_nextAllowedMs;
            if (!bot)
                return false;

            ObjectGuid guid = bot->GetGUID();
            auto it = s_nextAllowedMs.find(guid);
            if (it != s_nextAllowedMs.end() && nowMs < it->second)
                return false;

            s_nextAllowedMs[guid] = nowMs + 8000; // conservative idle throttle
            return true;
        }
    }

    HunterNonCombatRotation& HunterNonCombatRotation::Instance()
    {
        static HunterNonCombatRotation s;
        return s;
    }

    uint32 HunterNonCombatRotation::SelectSpell(RotationContext const& ctx)
    {
        if (!ctx.Bot)
            return 0;

        // Do nothing in combat.
        if (ctx.Bot->IsInCombat() || ctx.Bot->GetVictim())
            return 0;

        // Shared eat/drink (items, safe gating).
        if (Playerbots::AI::Combat::NonCombat::TryEatDrink(ctx.Bot))
            return 0;

        if (!ctx.Catalog)
            return 0;

        // Keep pet out while resting (human-like), without spam.
        if (!ctx.Bot->GetPet())
        {
            uint32 nowMs = getMSTime();
            if (CanTryCallPetNow(ctx.Bot, nowMs))
            {
                if (uint32 callPet = FindBestKnownSpellWithEffect(ctx.Bot, SPELL_EFFECT_SUMMON_PET))
                    return callPet;
            }
        }

        // Self-only buffs (spellbook-driven).
        if (uint32 selfBuff = ctx.Catalog->BestMissingSelfOnlyBuff(ctx.Bot))
            return selfBuff;

        // Master-only group buff (conservative).
        if (ctx.Master && ctx.Master->IsAlive())
        {
            float safeRadius = sConfigMgr->GetFloatDefault("Playerbots.NonCombat.EatDrink.SafeEnemyRadius", 30.0f);
            if (safeRadius < 5.0f) safeRadius = 5.0f;
            if (safeRadius > 80.0f) safeRadius = 80.0f;

            if (!Playerbots::AI::Combat::NonCombat::HasNearbyHostile(ctx.Bot, safeRadius))
            {
                uint32 nowMs = getMSTime();
                if (!Playerbots::AI::Combat::NonCombat::IsOnBuffCooldown(ctx.Bot, ctx.Master, nowMs))
                {
                    if (uint32 groupBuff = ctx.Catalog->BestMissingGroupBuffForTarget(ctx.Bot, ctx.Master))
                    {
                        Playerbots::AI::Combat::NonCombat::SetBuffCooldown(ctx.Bot, ctx.Master, nowMs, 5000);
                        return groupBuff;
                    }
                    Playerbots::AI::Combat::NonCombat::SetBuffCooldown(ctx.Bot, ctx.Master, nowMs, 3000);
                }
            }
        }

        return 0;
    }
}