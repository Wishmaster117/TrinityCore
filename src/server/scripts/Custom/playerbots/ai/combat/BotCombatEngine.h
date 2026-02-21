#ifndef PLAYERBOTS_AI_COMBAT_BOTCOMBATENGINE_H
#define PLAYERBOTS_AI_COMBAT_BOTCOMBATENGINE_H

#include <array>
#include <list>
#include "Config.h"
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
#include "playerbots/ai/combat/policy/CombatRolePolicy.h"
#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/INonCombatRotation.h"
#include "playerbots/ai/combat/rotations/RotationRegistry.h"
#include "playerbots/ai/combat/rotations/RotationContext.h"

namespace Playerbots::AI::Combat
{
    class BotCombatEngine final
    {
    public:
        // Phase 3.1: movement is role-driven.
        bool ShouldKeepRangedDistance() const { return _settings.KeepRangedDistance; }
        float GetPreferredRangedDistance() const { return _settings.PreferredRangeYards; } // 0 => melee

        // Phase 3.2.1: allow PlayerbotAI to ask the engine for an alternate victim (OT add).
        Unit* SelectCombatVictim(Player* bot, Player* master, Unit* masterVictim)
        {
            return SelectOffTankAddTarget(bot, master, masterVictim);
        }

        void Update(Player* bot, Player* master, Unit* victim, uint32 diff)
        {
            if (!bot || !master || !victim)
                return;

            Tick(diff);
            RefreshConfigIfNeeded();

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

                RefreshRoleAndSettings(bot);
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

            bool masterIsTank = IsInTankMode(master);

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
                if (TryDispel(bot, master, masterIsTank))
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
                if (nearby >= _settings.AoeMinTargets)
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
                // Phase 3.1: role-gated taunt.
                if (!_settings.AllowTaunt)
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
                        // Phase 3.2:
                        // - If master is tank-mode, master is MT, bot behaves as Off-Tank:
                        //   taunt only to peel fragile targets (heuristic) and avoid ping-pong.
                        if (!CanTauntVictim(victim))
                        {
                            // skip (lockout active)
                        }
                        else if (masterIsTank)
                        {
                            // As OT: don't taunt off the MT; only peel if the mob is on a fragile non-MT.
                            if (!victimOnMaster && IsFragilePlayerTarget(vv))
                            {
                                if (TryCastBestFromList(bot, victim, _catalog.GetTaunts(), 2))
                                {
                                    SetTauntLockout(victim);
                                    _castAttemptCooldownMs = kCastAttemptCooldownMs;
                                    return;
                                }
                            }
                        }
                        else
                        {
                            // As MT (no tank-mode master): classic "take it back" taunt.
                            if (TryCastBestFromList(bot, victim, _catalog.GetTaunts(), 2))
                            {
                                SetTauntLockout(victim);
                                _castAttemptCooldownMs = kCastAttemptCooldownMs;
                                return;
                            }
                        }
                    }
                }
            }

            // 5) Heal (self/master/group lowest HP).
            if (_catalog.HasHeals())
            {
                Unit* healTarget = SelectGroupHealTarget(bot, master, masterIsTank);
                if (healTarget && TryCastBestFromList(bot, healTarget, _catalog.GetHeals(), 4))
                {
                    _castAttemptCooldownMs = kCastAttemptCooldownMs;
                    return;
                }
            }

            // Phase 3.3: class rotation hook (per class file, no godfiles).
            {
                Rotations::RotationContext rctx;
                rctx.Bot = bot;
                rctx.Master = master;
                rctx.Victim = victim;
                rctx.Catalog = &_catalog;
                rctx.MasterIsTank = masterIsTank;

                if (Rotations::ICombatRotation* rot = Rotations::RotationRegistry::GetCombatRotation(bot))
                {
                    if (uint32 spellId = rot->SelectSpell(rctx))
                    {
                        if (VerifySpellCast(bot, spellId, victim))
                        {
                            uint32 rank = ResolveHighestKnownRank(bot, spellId);
                            if (rank)
                            {
                                bot->CastSpell(victim, rank, false);
                                _castAttemptCooldownMs = kCastAttemptCooldownMs;
                                return;
                            }
                        }
                    }
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
        static constexpr uint32 kTauntLockoutMs = 8000;

        static uint32 CountEnemiesAttacking(Player* bot, Unit* target, float radius)
        {
            if (!bot || !target)
                return 0;

            std::list<Unit*> enemies;
            CollectNearbyEnemies(bot, enemies, radius);

            uint32 count = 0;
            for (Unit* u : enemies)
            {
                if (!u || !u->IsAlive())
                    continue;

                Unit* vv = u->GetVictim();
                if (vv == target)
                    ++count;
            }

            return count;
        }

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

            if (_confRefreshMs > diff)
                _confRefreshMs -= diff;
            else
                _confRefreshMs = 0;

            for (auto& e : _tauntLocks)
            {
                if (e.Guid.IsEmpty())
                    continue;
                if (e.Ms > diff)
                    e.Ms -= diff;
                else
                {
                    e.Ms = 0;
                    e.Guid.Clear();
                }
            }

            if (_otTargetMs > 0)
            {
                if (_otTargetMs > diff)
                    _otTargetMs -= diff;
                else
                    _otTargetMs = 0;
            }
        }

        void RefreshConfigIfNeeded()
        {
            if (_confRefreshMs > 0)
                return;

            int32 v = sConfigMgr->GetIntDefault("Playerbots.Combat.OffTankOverwhelmedMobs", 3);
            if (v < 1) v = 1;
            if (v > 10) v = 10;
            _mtOverwhelmedMinMobs = uint32(v);

            _confRefreshMs = 5000; // refresh every 5s (supports .reload config)
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

        void RefreshRoleAndSettings(Player* bot)
        {
            if (!bot)
                return;

            bool inTankMode = IsInTankMode(bot);
            _role = Policy::CombatRolePolicy::DetermineRole(
                inTankMode,
                _catalog.HasTaunts(),
                _catalog.HasMelee(),
                _catalog.HasRangedOffense(),
                _catalog.HasHeals(),
                _catalog.HealCount(),
                _catalog.OffensiveCount());

            _settings = Policy::CombatRolePolicy::GetSettings(_role, _catalog.HasRangedOffense());
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
                if (!u || !u->IsAlive() || !bot->IsValidAttackTarget(u))
                    return true;

                // Exclude player-controlled units (pets/totems/guardians), but keep players.
                if (!u->IsPlayer() && u->IsControlledByPlayer())
                    return true;

                return false;
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

        bool TryDispel(Player* bot, Player* master, bool masterIsTank)
        {
            if (!_catalog.HasDispels() || !bot || !master || !bot->GetMap())
                return false;

            Difficulty diff = Difficulty(bot->GetMap()->GetDifficultyID());

            // Candidate targets: bot, master, group.
            std::vector<Player*> friends;
            // Phase 3.2: if master is MT, prioritize master first.
            if (masterIsTank)
            {
                friends.push_back(master);
                friends.push_back(bot);
            }
            else
            {
                friends.push_back(bot);
                friends.push_back(master);
            }

            // Phase 3.1: only healer dispels the whole group (generic role setting).
            if (_settings.DispelGroup)
            {
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

        Unit* SelectGroupHealTarget(Player* bot, Player* master, bool masterIsTank) const
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
                uint8 effective = hp;

                // Phase 3.2: prioritize the MT (master tank-mode) slightly.
                if (masterIsTank && p == master && hp < 98)
                {
                    // bias by 5% to pick master earlier when close.
                    effective = uint8((hp > 5) ? (hp - 5) : 0);
                }

                if (effective >= bestPct)
                    return;

                // Don’t heal tiny scratches; keeps casts useful.
                if (hp > _settings.HealThresholdPct)
                if (hp > 85)
                    return;

                best = p;
                bestPct = effective;
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

        Unit* SelectOffTankAddTarget(Player* bot, Player* master, Unit* masterVictim)
        {
            if (!bot || !master || !masterVictim || !masterVictim->IsAlive())
                return masterVictim;

            // Micro-step: Off-tank must not start pulls.
            // Only allow OT add logic once the master has engaged combat.
            if (!master->IsInCombat() && !master->GetVictim())
                return masterVictim;

            // Only OT logic when:
            // - master is MT (tank-mode)
            // - bot is a tank (AllowTaunt in our role settings)
            bool masterIsTank = IsInTankMode(master);
            if (!masterIsTank || !_settings.AllowTaunt)
                return masterVictim;

            // Only peel adds if MT is overwhelmed (too many mobs on master).
            uint32 mobsOnMt = CountEnemiesAttacking(bot, master, 30.0f);
            if (mobsOnMt < _mtOverwhelmedMinMobs)
                return masterVictim;

            // If we have a cached OT target and it's still valid, keep it (avoid target thrash).
            if (!_otTargetGuid.IsEmpty() && _otTargetMs > 0)
            {
                if (Unit* cached = ObjectAccessor::GetUnit(*bot, _otTargetGuid))
                {
                    if (cached->IsAlive() && bot->IsValidAttackTarget(cached) && cached != masterVictim)
                        return cached;
                }
            }

            // Look for nearby enemies; if there is only one, do not OT.
            std::list<Unit*> enemies;
            CollectNearbyEnemies(bot, enemies, 30.0f);
            if (enemies.size() < 2)
                return masterVictim;

            // Pick best add:
            // 1) not the master victim
            // 2) attacking a fragile group member (mana/low hp) is preferred
            // 3) otherwise closest
            Unit* best = nullptr;
            uint32 bestScore = 0;

            for (Unit* u : enemies)
            {
                if (!u || !u->IsAlive() || u == masterVictim)
                    continue;
            
                if (!bot->IsValidAttackTarget(u))
                    continue;
            
                // Only pick adds that are already engaged with a player.
                Unit* vv = u->GetVictim();
                if (!vv || !vv->IsPlayer())
                    continue;
            
                // Don't peel mobs already on MT (master). We want mobs hitting healer/dps.
                if (vv == master)
                    continue;
            
                uint32 score = 0;
            
                // Prefer peeling fragile targets (mana user / low hp).
                if (IsFragilePlayerTarget(vv))
                    score += 250;
                else
                    score += 80; // still peel DPS if MT overwhelmed
            
                // If victim is low, prioritize peel.
                if (vv->HealthBelowPct(60))
                    score += 80;
            
                // Closeness bonus (closer is better).
                float dist = bot->GetDistance(u);
                if (dist < 30.0f)
                {
                    uint32 closeBonus = uint32((30.0f - dist) * 3.0f);
                    score += closeBonus;
                }
            
                if (!best || score > bestScore)
                {
                    best = u;
                    bestScore = score;
                }
            }

            if (!best)
                return masterVictim;

            // Cache OT target for a short time to avoid oscillations.
            _otTargetGuid = best->GetGUID();
            _otTargetMs = 4000; // 4s lock
            return best;
        }

        static void CollectNearbyEnemies(Player* bot, std::list<Unit*>& out, float radius)
        {
            out.clear();
            if (!bot)
                return;

            Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(bot, bot, radius);
            Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, out, check);
            Cell::VisitAllObjects(bot, searcher, radius);

            out.remove_if([bot](Unit* u)
            {
                if (!u || !u->IsAlive() || !bot->IsValidAttackTarget(u))
                    return true;

                // Exclude player-controlled units (pets/totems/guardians), but keep players.
                if (!u->IsPlayer() && u->IsControlledByPlayer())
                    return true;

                return false;
            });
        }

        // Heuristic: treat low-armor/caster/healer types as "fragile" without hardcoding classes.
        // - mana users tend to be casters/healers
        // - low health pct => emergency peel
        static bool IsFragilePlayerTarget(Unit* u)
        {
            Player* p = u ? u->ToPlayer() : nullptr;
            if (!p)
                return false;

            if (p->HealthBelowPct(55))
                return true;

            // Power type can be used as generic signal (mana casters/healers).
            return p->GetPowerType() == POWER_MANA;
        }

        struct TauntLockEntry
        {
            ObjectGuid Guid;
            uint32 Ms = 0;
        };

        bool CanTauntVictim(Unit* victim) const
        {
            if (!victim)
                return false;

            for (TauntLockEntry const& e : _tauntLocks)
            {
                if (!e.Guid.IsEmpty() && e.Guid == victim->GetGUID() && e.Ms > 0)
                    return false;
            }

            return true;
        }

        void SetTauntLockout(Unit* victim)
        {
            if (!victim)
                return;

            // Find empty slot or replace the oldest (simple: first empty, else slot 0).
            for (auto& e : _tauntLocks)
            {
                if (e.Guid.IsEmpty())
                {
                    e.Guid = victim->GetGUID();
                    e.Ms = kTauntLockoutMs;
                    return;
                }
            }

            _tauntLocks[0].Guid = victim->GetGUID();
            _tauntLocks[0].Ms = kTauntLockoutMs;
        }

        SpellCatalog _catalog;
        bool _builtOnce = false;

        Policy::Role _role = Policy::Role::Dps;
        Policy::Settings _settings;

        // Phase 3.2: avoid taunt ping-pong per mob.
        std::array<TauntLockEntry, 8> _tauntLocks{};

        // Phase 3.2.1: OT target cache to avoid oscillations.
        ObjectGuid _otTargetGuid;
        uint32 _otTargetMs = 0;

        // Phase 3.2.1: configurable threshold (mobs on MT) before OT starts peeling.
        uint32 _mtOverwhelmedMinMobs = 3;
        uint32 _confRefreshMs = 0;

        uint32 _castAttemptCooldownMs = 0;
        uint32 _rebuildDelayMs = 0;
        uint32 _rebuildTimerMs = 0;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_BOTCOMBATENGINE_H