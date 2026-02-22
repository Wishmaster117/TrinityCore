#include "playerbots/ai/combat/classes/warlock/WarlockNonCombatRotation.h"

#include "playerbots/ai/combat/rotations/RotationContext.h"
#include "playerbots/ai/combat/SpellCatalog.h"
#include "playerbots/ai/combat/noncombat/EatDrinkHelper.h"
#include "playerbots/ai/combat/noncombat/NonCombatBuffHelper.h"

#include "Config.h"
#include "Player.h"
#include "Timer.h"

namespace Playerbots::AI::Combat::Rotations
{
    WarlockNonCombatRotation& WarlockNonCombatRotation::Instance()
    {
        static WarlockNonCombatRotation s;
        return s;
    }

    uint32 WarlockNonCombatRotation::SelectSpell(RotationContext const& ctx)
    {
        if (!ctx.Bot)
            return 0;

        // Human-like: do nothing in combat.
        if (ctx.Bot->IsInCombat() || ctx.Bot->GetVictim())
            return 0;

        // Shared eat/drink (items, safe gating).
        if (Playerbots::AI::Combat::NonCombat::TryEatDrink(ctx.Bot))
            return 0;

        if (!ctx.Catalog)
            return 0;

        // Self-only buffs (Demon Armor-like), spellbook-driven.
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