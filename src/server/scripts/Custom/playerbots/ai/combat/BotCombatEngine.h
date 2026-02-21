#ifndef PLAYERBOTS_AI_COMBAT_BOTCOMBATENGINE_H
#define PLAYERBOTS_AI_COMBAT_BOTCOMBATENGINE_H

#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
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

            // Phase 2 order:
            // 1) Interrupt 2) Dispel 3) Buff 4) AOE 5) Heal (then melee/ranged DPS)

            // 1) Interrupt if victim is casting.
            if (_catalog.HasInterrupts() && victim->IsNonMeleeSpellCast(false))
            {
                if (TryCastBestFromList(bot, victim, _catalog.GetInterrupts(), 3))
                {
                    _castAttemptCooldownMs = kCastAttemptCooldownMs;
                    return;
                }
            }

            // 2) Dispel bot/master/group if possible.
            if (_catalog.HasDispels())
            {
                if (TryDispel(bot, master))
                {
                    _castAttemptCooldownMs = kCastAttemptCooldownMs;
                    return;
                }
            }

            // 3) Buffs (self/master/group if missing).
            if (_catalog.HasBuffs())
            {
                if (TryBuff(bot, master))
                {
                    _castAttemptCooldownMs = kCastAttemptCooldownMs;
                    return;
                }
            }

            // 4) AOE if enough enemies around.
            if (_catalog.HasAoe())
            {
                uint32 nearby = CountNearbyEnemies(bot, 10.0f);
                if (nearby >= 3)
                {
                    if (TryCastBestFromList(bot, victim, _catalog.GetAoe(), 4))
                    {
                        _castAttemptCooldownMs = kCastAttemptCooldownMs;
                        return;
                    }
                }
            }

            // 4.5) Opportunistic CC (very conservative).
            if (_catalog.HasCrowdControls())
            {
                // If bot is in trouble and the victim is targeting the bot, try CC.
                if (bot->HealthBelowPct(40) && victim->GetVictim() == bot)
                {
                    if (TryCastBestFromList(bot, victim, _catalog.GetCrowdControls(), 3))
                    {
                        _castAttemptCooldownMs = kCastAttemptCooldownMs;
                        return;
                    }
                }
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

            // 5) Heal (self/master/group lowest HP).
            if (_catalog.HasHeals())
            {
                Unit* healTarget = SelectGroupHealTarget(bot, master);
                if (healTarget && TryCastBestFromList(bot, healTarget, _catalog.GetHeals(), 4))
                {
                    _castAttemptCooldownMs = kCastAttemptCooldownMs;
                    return;
                }
            }

            // DPS fallback: melee abilities then ranged offense.
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

        static uint32 CountNearbyEnemies(Player* bot, float radius)
        {
            if (!bot)
                return 0;

            std::list<Unit*> targets;
            Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(bot, bot, radius);
            Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, targets, check);
            Cell::VisitAllObjects(bot, searcher, radius);

            // remove non-attackable junk
            targets.remove_if([bot](Unit* u)
            {
                return !u || !u->IsAlive() || !bot->IsValidAttackTarget(u);
            });

            return uint32(targets.size());
        }

        static bool HasAuraFromSpellChain(Unit* target, uint32 spellId)
        {
            if (!target || !spellId)
                return false;

            uint32 first = sSpellMgr->GetFirstSpellInChain(spellId);
            if (!first)
                first = spellId;

            for (uint32 id = first; id; id = sSpellMgr->GetNextSpellInChain(id))
            {
                if (target->HasAura(id))
                    return true;
            }

            return false;
        }

        bool TryDispel(Player* bot, Player* master)
        {
            if (!_catalog.HasDispels() || !bot || !master || !bot->GetMap())
                return false;

            Difficulty diff = Difficulty(bot->GetMap()->GetDifficultyID());

            // Candidate targets: bot, master, group.
            std::vector<Player*> friends;
            friends.push_back(bot);
            friends.push_back(master);

            if (Group* g = master->GetGroup())
            {
                for (Group::MemberSlot const& slot : g->GetMemberSlots())
                {
                    if (slot.guid.IsEmpty())
                        continue;
                    if (Player* member = ObjectAccessor::FindPlayer(slot.guid))
                        friends.push_back(member);
                }
            }

            for (uint32 dispelSpellId : _catalog.GetDispels())
            {
                SpellInfo const* dispelInfo = sSpellMgr->GetSpellInfo(dispelSpellId, diff);
                if (!dispelInfo)
                    continue;

                uint32 dispelMask = dispelInfo->GetDispelMask();
                if (!dispelMask)
                    continue;

                for (Player* target : friends)
                {
                    if (!target || !target->IsAlive())
                        continue;

                    DispelChargesList dispelList;
                    target->GetDispellableAuraList(bot, dispelMask, dispelList);
                    if (dispelList.empty())
                        continue;

                    if (VerifySpellCast(bot, dispelSpellId, target))
                    {
                        uint32 rank = ResolveHighestKnownRank(bot, dispelSpellId);
                        if (rank)
                        {
                            bot->CastSpell(target, rank, false);
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        bool TryBuff(Player* bot, Player* master)
        {
            if (!_catalog.HasBuffs() || !bot || !master)
                return false;

            std::vector<Player*> friends;
            friends.push_back(bot);
            friends.push_back(master);

            if (Group* g = master->GetGroup())
            {
                for (Group::MemberSlot const& slot : g->GetMemberSlots())
                {
                    if (slot.guid.IsEmpty())
                        continue;
                    if (Player* member = ObjectAccessor::FindPlayer(slot.guid))
                        friends.push_back(member);
                }
            }

            for (uint32 buffSpellId : _catalog.GetBuffs())
            {
                for (Player* target : friends)
                {
                    if (!target || !target->IsAlive())
                        continue;

                    if (HasAuraFromSpellChain(target, buffSpellId))
                        continue;

                    if (!VerifySpellCast(bot, buffSpellId, target))
                        continue;

                    uint32 rank = ResolveHighestKnownRank(bot, buffSpellId);
                    if (!rank)
                        continue;

                    bot->CastSpell(target, rank, false);
                    return true;
                }
            }

            return false;
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