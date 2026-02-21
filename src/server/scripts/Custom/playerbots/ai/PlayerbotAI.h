/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_AI_PLAYERBOTAI_H
#define PLAYERBOTS_AI_PLAYERBOTAI_H

#include "Player.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Unit.h"

namespace Playerbots::AI
{
    class PlayerbotAI final
    {
    public:
        enum class State : uint8
        {
            Idle   = 0,
            Follow = 1,
            Combat = 2
        };

        static Unit* ResolveAssistTarget(Player* master)
        {
            if (!master)
                return nullptr;

            // 1) Victim is only set when auto-attack is engaged (not always true on retail).
            if (Unit* v = master->GetVictim())
                return v;

            // 2) Fallback: use current selected target (common for spell pulls).
            ObjectGuid targetGuid = master->GetTarget();
            if (!targetGuid)
                return nullptr;

            return ObjectAccessor::GetUnit(*master, targetGuid);
        }

        void Update(Player* bot, Player* master, uint32 diff)
        {
            if (!bot || !master)
            {
                _state = State::Idle;
                return;
            }

            if (!bot->IsInWorld() || !master->IsInWorld())
            {
                _state = State::Idle;
                return;
            }

            if (!bot->IsAlive() || !master->IsAlive())
            {
                // Keep state but don't do anything.
                return;
            }

            TickCooldowns(diff);

            // Decide desired state.
            Unit* victim = ResolveAssistTarget(master);
            bool canCombat = victim && victim->IsAlive()
                && bot->GetMap() == master->GetMap()
                // Only assist when the master is actually fighting (prevents accidental "attack my target" when not pulling).
                && (master->IsInCombat() || victim->GetVictim() == master);

            if (canCombat)
            {
                if (_state != State::Combat)
                    _state = State::Combat;
            }
            else
            {
                if (_state == State::Combat)
                    bot->AttackStop();

                _state = State::Follow;
            }

            switch (_state)
            {
                case State::Idle:
                    // Nothing (no leader).
                    break;

                case State::Follow:
                    // Movement/follow is handled by the existing Playerbots::Manager follow logic.
                    // Keep minimal to avoid fighting MotionMaster.
                    break;

                case State::Combat:
                {
                    if (!victim || !victim->IsAlive())
                    {
                        bot->AttackStop();
                        _state = State::Follow;
                        break;
                    }

                    // Minimal combat assist: chase + melee auto-attack on master's victim.
                    // 1) Ensure we run towards the victim if not in melee range.
                    if (!bot->IsWithinMeleeRange(victim))
                        bot->GetMotionMaster()->MoveChase(victim);

                    // 2) Ensure auto-attack is enabled.
                    if (bot->GetVictim() != victim)
                        bot->Attack(victim, true);

                    // 3) Minimal spellcasting: one offensive spell per class (auto-detected from known spells)
                    //    + one heal if the bot has a reasonable healing spell.
                    //
                    // NOTE: Keep this intentionally conservative:
                    // - don't fight GCD/cast state
                    // - don't cast while moving (reduces failed casts/spam)
                    // - throttle attempts
                    DoSpellcasting(bot, master, victim);

                    break;
                }
            }
        }

        State GetState() const { return _state; }
        bool IsInCombat() const { return _state == State::Combat; }

    private:
        static constexpr uint32 kCastAttemptCooldownMs = 1500;

        static bool HasDirectDamageEffect(SpellInfo const* spellInfo)
        {
            if (!spellInfo)
                return false;

            for (SpellEffectInfo const& eff : spellInfo->GetEffects())
            {
                switch (eff.Effect)
                {
                    case SPELL_EFFECT_SCHOOL_DAMAGE:
                    case SPELL_EFFECT_WEAPON_DAMAGE:
                    case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                    case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                    case SPELL_EFFECT_HEALTH_LEECH:
                        return true;
                    default:
                        break;
                }
            }
            return false;
        }

        static bool HasHealingEffect(SpellInfo const* spellInfo)
        {
            if (!spellInfo)
                return false;

            for (SpellEffectInfo const& eff : spellInfo->GetEffects())
            {
                switch (eff.Effect)
                {
                    case SPELL_EFFECT_HEAL:
                    case SPELL_EFFECT_HEAL_PCT:
                    case SPELL_EFFECT_HEAL_MAX_HEALTH:
                        return true;
                    default:
                        break;
                }
            }
            return false;
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
            if (!bot || !spellId || !target)
                return false;

            uint32 knownRank = ResolveHighestKnownRank(bot, spellId);
            if (!knownRank)
                return false;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(knownRank, bot->GetMap()->GetDifficultyID());
            if (!spellInfo)
                return false;

            if (spellInfo->IsPassive())
                return false;

            if (bot->GetSpellHistory()->HasGlobalCooldown(spellInfo))
                return false;

            // Use Spell::CanAutoCast as a conservative validator (range, LOS, state, power, etc.).
            Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);
            bool ok = spell->CanAutoCast(target);
            delete spell;
            return ok;
        }

        static uint32 FindBestNukeSpell(Player* bot)
        {
            if (!bot)
                return 0;

            uint32 best = 0;
            float bestRange = 0.0f;
            uint32 bestCastTime = 0;

            for (auto const& [spellId, data] : bot->GetSpellMap())
            {
                if (data.state == PLAYERSPELL_REMOVED || data.disabled)
                    continue;

                SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, bot->GetMap()->GetDifficultyID());
                if (!info || info->IsPassive())
                    continue;

                // Prefer a simple, direct damage spell ("nuke") that targets enemies.
                if (info->IsPositive() || !HasDirectDamageEffect(info))
                    continue;

                float range = info->GetMaxRange(false, bot);
                // Skip melee-ish abilities here; melee is already handled by auto-attack.
                if (range <= 6.0f)
                    continue;

                uint32 castTime = info->CalcCastTime();

                // Heuristic: prefer longer range, then faster cast.
                if (!best || range > bestRange || (range == bestRange && castTime < bestCastTime))
                {
                    best = spellId;
                    bestRange = range;
                    bestCastTime = castTime;
                }
            }

            return best;
        }

        static uint32 FindBestHealSpell(Player* bot)
        {
            if (!bot)
                return 0;

            uint32 best = 0;
            uint32 bestCastTime = 0;
            float bestRange = 0.0f;

            for (auto const& [spellId, data] : bot->GetSpellMap())
            {
                if (data.state == PLAYERSPELL_REMOVED || data.disabled)
                    continue;

                SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, bot->GetMap()->GetDifficultyID());
                if (!info || info->IsPassive())
                    continue;

                if (!info->IsPositive() || !HasHealingEffect(info))
                    continue;

                float range = info->GetMaxRange(true, bot);
                if (range <= 0.0f)
                    continue;

                uint32 castTime = info->CalcCastTime();

                // Heuristic: prefer faster, then longer range.
                if (!best || castTime < bestCastTime || (castTime == bestCastTime && range > bestRange))
                {
                    best = spellId;
                    bestCastTime = castTime;
                    bestRange = range;
                }
            }

            return best;
        }

        void DoSpellcasting(Player* bot, Player* master, Unit* victim)
        {
            if (!bot || !master || !victim)
                return;

            // Throttle attempts.
            if (_castAttemptCooldownMs > 0)
                return;

            // Avoid spamming while already casting or moving.
            if (bot->IsNonMeleeSpellCast(false) || bot->isMoving())
            {
                _castAttemptCooldownMs = 500;
                return;
            }

            if (!_cachedNukeSpell)
                _cachedNukeSpell = FindBestNukeSpell(bot);

            if (!_cachedHealSpell)
                _cachedHealSpell = FindBestHealSpell(bot);

            // 1) Heal master (or self) if we can and someone is in trouble.
            if (_cachedHealSpell)
            {
                Unit* healTarget = nullptr;

                if (bot->HealthBelowPct(60))
                    healTarget = bot;
                else if (master->HealthBelowPct(70))
                    healTarget = master;

                if (healTarget && VerifySpellCast(bot, _cachedHealSpell, healTarget))
                {
                    bot->CastSpell(healTarget, ResolveHighestKnownRank(bot, _cachedHealSpell), false);
                    _castAttemptCooldownMs = kCastAttemptCooldownMs;
                    return;
                }
            }

            // 2) Otherwise: cast our best "nuke" on the victim.
            if (_cachedNukeSpell && VerifySpellCast(bot, _cachedNukeSpell, victim))
            {
                bot->CastSpell(victim, ResolveHighestKnownRank(bot, _cachedNukeSpell), false);
                _castAttemptCooldownMs = kCastAttemptCooldownMs;
                return;
            }

            // If nothing was cast, still throttle a bit to avoid expensive scans every tick.
            _castAttemptCooldownMs = kCastAttemptCooldownMs;
        }

        void TickCooldowns(uint32 diff)
        {
            if (_castAttemptCooldownMs > diff)
                _castAttemptCooldownMs -= diff;
            else
                _castAttemptCooldownMs = 0;
        }

        State _state = State::Idle;

        // Cached spell choices (auto-detected from the bot's known spells).
        uint32 _cachedNukeSpell = 0;
        uint32 _cachedHealSpell = 0;

        // Lightweight throttling to avoid spam.
        uint32 _castAttemptCooldownMs = 0;
    };
}

#endif // PLAYERBOTS_AI_PLAYERBOTAI_H