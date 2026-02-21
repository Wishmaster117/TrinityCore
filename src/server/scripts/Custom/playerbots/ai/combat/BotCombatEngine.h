#ifndef PLAYERBOTS_AI_COMBAT_BOTCOMBATENGINE_H
#define PLAYERBOTS_AI_COMBAT_BOTCOMBATENGINE_H

#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Spell.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellAuraDefines.h"
#include "Unit.h"

#include <cstdint>
#include <vector>

#include "playerbots/ai/combat/CombatContext.h"
#include "playerbots/ai/combat/SpellCatalog.h"

namespace Playerbots::AI::Combat
{
    class BotCombatEngine final
    {
    public:
        // Phase 1.2: if we have ranged offense, keep distance instead of hard melee chase.
        bool ShouldKeepRangedDistance() const { return _catalog.HasRangedOffense(); }
        float GetPreferredRangedDistance() const { return 25.0f; } // yards, conservative default

        void Update(Player* bot, Player* master, Unit* victim, uint32 diff)
        {
            if (!bot || !master || !victim)
                return;

            Tick(diff);

            // Lazy / periodic rebuild of spell catalog (perf-friendly).
            if (!_builtOnce || _catalog.NeedsRebuild(bot) || _rebuildTimerMs == 0)
            {
                if (_rebuildDelayMs == 0)
                    _rebuildDelayMs = 250; // avoid rebuilding on the exact combat entry tick for many bots
            }

            if (_rebuildDelayMs == 0 && (!_builtOnce || _catalog.NeedsRebuild(bot) || _rebuildTimerMs == 0))
            {
                _catalog.Build(bot);
                _builtOnce = true;
                _rebuildTimerMs = kPeriodicRebuildMs;
            }

            if (_castAttemptCooldownMs > 0)
                return;

            CombatContext ctx;
            BuildContext(ctx, bot, master, victim);

            // Conservative gates.
            if (ctx.IsCasting || ctx.IsMoving)
            {
                _castAttemptCooldownMs = 500;
                return;
            }

            // 0) Taunt if we can and someone else is tanking the victim (basic aggro help).
            if (_catalog.HasTaunts())
            {
                // Guard-rail: only taunt while in "tank mode" (generic detection via threat-mod aura).
                if (!IsInTankMode(bot))
                {
                    // Don't block other actions; only skip taunt.
                }
                else
                if (Unit* vv = victim->GetVictim())
                {
                    bool victimOnBot = (vv == bot);
                    bool victimOnMaster = (vv == master);
                    bool victimOnGroup = false;

                    if (!victimOnBot && !victimOnMaster)
                    {
                        if (Group* g = master->GetGroup())
                        {
                            for (Group::MemberSlot const& slot : g->GetMemberSlots())
                            {
                                if (slot.guid.IsEmpty())
                                    continue;

                                Player* member = ObjectAccessor::FindPlayer(slot.guid);
                                if (member && member->IsAlive() && vv == member)
                                {
                                    victimOnGroup = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (!victimOnBot && (victimOnMaster || victimOnGroup))
                    {
                        if (TryCastBestFromList(bot, victim, _catalog.GetTaunts(), 2))
                        {
                            _castAttemptCooldownMs = kCastAttemptCooldownMs;
                            return;
                        }
                    }
                }
            }

            // 1) Heals (self/master/group lowest HP).
            if (_catalog.HasHeals())
            {
                Unit* healTarget = SelectGroupHealTarget(bot, master);
                if (healTarget && TryCastBestFromList(bot, healTarget, _catalog.GetHeals(), 4))
                {
                    _castAttemptCooldownMs = kCastAttemptCooldownMs;
                    return;
                }
            }

            // 2) Melee abilities if we are in melee range (rogue/war/etc).
            if (_catalog.HasMelee() && bot->IsWithinMeleeRange(victim))
            {
                if (TryCastBestFromList(bot, victim, _catalog.GetMelee(), 6))
                {
                    _castAttemptCooldownMs = kCastAttemptCooldownMs;
                    return;
                }
            }

            // 3) Ranged offensive. 
            if (_catalog.HasOffensive())
            {
                if (TryCastBestFromList(bot, victim, _catalog.GetOffensive(), 6))
                {
                    _castAttemptCooldownMs = kCastAttemptCooldownMs;
                    return;
                }
            }

            _castAttemptCooldownMs = kCastAttemptCooldownMs;
        }

    private:
        static constexpr uint32 kCastAttemptCooldownMs = 1200;
        static constexpr uint32 kPeriodicRebuildMs = 30000;

        void Tick(uint32 diff)
        {
            if (_castAttemptCooldownMs > diff)
                _castAttemptCooldownMs -= diff;
            else
                _castAttemptCooldownMs = 0;

            if (_rebuildDelayMs > diff)
                _rebuildDelayMs -= diff;
            else
                _rebuildDelayMs = 0;

            if (_rebuildTimerMs > diff)
                _rebuildTimerMs -= diff;
            else
                _rebuildTimerMs = 0;
        }

        static void BuildContext(CombatContext& ctx, Player* bot, Player* master, Unit* victim)
        {
            ctx.Bot = bot;
            ctx.Master = master;
            ctx.Victim = victim;
            ctx.IsCasting = bot->IsNonMeleeSpellCast(false);
            ctx.IsMoving = bot->isMoving();
            ctx.BotNeedsHeal = bot->HealthBelowPct(60);
            ctx.MasterNeedsHeal = master->HealthBelowPct(70);
        }

        static Unit* SelectGroupHealTarget(Player* bot, Player* master)
        {
            if (!bot || !master)
                return nullptr;

            Unit* best = nullptr;
            uint8 bestPct = 101;

            auto consider = [&](Player* p)
            {
                if (!p || !p->IsAlive())
                    return;

                uint8 hp = uint8(p->GetHealthPct());
                if (hp >= bestPct)
                    return;

                // Don’t heal tiny scratches; keeps casts useful.
                if (hp > 85)
                    return;

                best = p;
                bestPct = hp;
            };

            // Always consider bot and master.
            consider(bot);
            consider(master);

            if (Group* g = master->GetGroup())
            {
                for (Group::MemberSlot const& slot : g->GetMemberSlots())
                {
                    if (slot.guid.IsEmpty())
                        continue;

                    Player* member = ObjectAccessor::FindPlayer(slot.guid);
                    consider(member);
                }
            }

            return best;
        }

        static uint32 ResolveHighestKnownRank(Player* bot, uint32 spellId)
        {
            if (!bot || !spellId)
                return 0;

            uint32 knownRank = 0;
            uint32 nextRank = 0;

            if (bot->HasSpell(spellId))
            {
                knownRank = spellId;
                nextRank = sSpellMgr->GetNextSpellInChain(spellId);
            }
            else
            {
                nextRank = sSpellMgr->GetFirstSpellInChain(spellId);
            }

            while (nextRank && bot->HasSpell(nextRank))
            {
                knownRank = nextRank;
                nextRank = sSpellMgr->GetNextSpellInChain(knownRank);
            }

            return knownRank;
        }

        static bool VerifySpellCast(Player* bot, uint32 spellId, Unit* target)
        {
            if (!bot || !spellId || !target || !bot->GetMap())
                return false;

            uint32 rank = ResolveHighestKnownRank(bot, spellId);
            if (!rank)
                return false;

            SpellInfo const* info = sSpellMgr->GetSpellInfo(rank, bot->GetMap()->GetDifficultyID());
            if (!info || info->IsPassive())
                return false;

            if (bot->GetSpellHistory()->HasGlobalCooldown(info))
                return false;

            // Conservative validation (range/LOS/state/power/etc).
            Spell* spell = new Spell(bot, info, TRIGGERED_NONE);
            bool ok = spell->CanAutoCast(target);
            delete spell;
            return ok;
        }

        static bool TryCastBestFromList(Player* bot, Unit* target, std::vector<uint32> const& spells, uint32 maxAttempts)
        {
            if (!bot || !target || spells.empty())
                return false;

            uint32 attempts = 0;
            for (uint32 spellId : spells)
            {
                if (!spellId)
                    continue;

                if (!VerifySpellCast(bot, spellId, target))
                {
                    if (++attempts >= maxAttempts)
                        break;
                    continue;
                }

                uint32 rank = ResolveHighestKnownRank(bot, spellId);
                if (!rank)
                {
                    if (++attempts >= maxAttempts)
                        break;
                    continue;
                }

                bot->CastSpell(target, rank, false);
                return true;
            }

            return false;
        }

        static bool IsInTankMode(Player* bot)
        {
            if (!bot)
                return false;

            // Generic heuristic: tank stances/forms/presences typically provide a MOD_THREAT aura.
            // This avoids hardcoding class spell ids and keeps behavior core-friendly.
            return bot->HasAuraType(SPELL_AURA_MOD_THREAT);
        }

        SpellCatalog _catalog;
        bool _builtOnce = false;

        uint32 _castAttemptCooldownMs = 0;
        uint32 _rebuildDelayMs = 0;
        uint32 _rebuildTimerMs = 0;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_BOTCOMBATENGINE_H