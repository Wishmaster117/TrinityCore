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

        void Update(Player* bot, Player* master, uint32 /*diff*/)
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

                    break;
                }
            }
        }

        State GetState() const { return _state; }
        bool IsInCombat() const { return _state == State::Combat; }

    private:
        State _state = State::Idle;
    };
}

#endif // PLAYERBOTS_AI_PLAYERBOTAI_H