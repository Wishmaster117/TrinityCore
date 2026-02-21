/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots_manager.h"

#include "Config.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"
#include "Unit.h"

#include "playerbots_registry.h"
#include "playerbots/ai/PlayerbotAI.h"
#include "playerbots/movement/playerbots_movement.h"
#include "playerbots/formation/playerbots_formation.h"
#include "playerbots/randombots/playerbots_rndbots_bootstrap.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace Playerbots
{
    // Private MVP state
    static bool _enabled = false;
    static uint32 _tickMs = 200;
    static uint32 _accumulated = 0;
    static bool _started = false;
    static uint32 _gcAccumulated = 0;

    static float _followDistance = 10.0f;
    static float _followAngle = 0.0f;
    static float _stopDistanceFactor = 0.80f; // hysteresis: stop when <= followDist*factor

    static uint32 _commandMinIntervalMs = 250;

    static std::unordered_map<ObjectGuid, Formation::Type> _formationByLeader;
    static std::unordered_map<ObjectGuid, std::unique_ptr<AI::PlayerbotAI>> _aiByBot;

    Manager& Manager::Instance()
    {
        static Manager instance;
        return instance;
    }

    bool Manager::IsEnabled() const { return _enabled; }
    uint32 Manager::GetTickMs() const { return _tickMs; }
    float Manager::GetFollowDistance() const { return _followDistance; }
    float Manager::GetFollowAngle() const { return _followAngle; }
    uint32 Manager::GetCommandMinIntervalMs() const { return _commandMinIntervalMs; }

    Formation::Type Manager::GetFormationFor(ObjectGuid leaderGuid) const
    {
        auto it = _formationByLeader.find(leaderGuid);
        if (it == _formationByLeader.end())
            return Formation::Type::Near; // default
        return it->second;
    }

    void Manager::SetFormationFor(ObjectGuid leaderGuid, Formation::Type type)
    {
        _formationByLeader[leaderGuid] = type;
        TC_LOG_INFO("server.loading", "Playerbots: runtime formation for leader {} -> {}",
            leaderGuid.ToString(), Formation::ToString(type));
    }

    void Manager::OnBotDespawned(ObjectGuid botGuid)
    {
        // Drop cached AI state for this bot.
        _aiByBot.erase(botGuid);

        // If the bot was a leader with a custom formation, drop it.
        _formationByLeader.erase(botGuid);
    }

    void Manager::SetEnabled(bool enabled)
    {
        _enabled = enabled;
        TC_LOG_INFO("server.loading", "Playerbots: runtime toggle -> Enable={}", _enabled);
    }

    void Manager::SetTickMs(uint32 tickMs)
    {
        if (tickMs < 50)
            tickMs = 50;

        _tickMs = tickMs;
        TC_LOG_INFO("server.loading", "Playerbots: runtime tick -> Tick={}ms", _tickMs);
    }

    void Manager::SetFollowDistance(float dist)
    {
        if (dist < 1.0f)
            dist = 1.0f;
        _followDistance = dist;
        TC_LOG_INFO("server.loading", "Playerbots: runtime follow distance -> {}", _followDistance);
    }

    void Manager::SetFollowAngle(float angle)
    {
        _followAngle = angle;
        TC_LOG_INFO("server.loading", "Playerbots: runtime follow angle -> {}", _followAngle);
    }

    void Manager::OnStartup()
    {
        if (_started)
            return;
        _started = true;

        ReloadConfig();

        // Random bot accounts bootstrap (auth + characters pool table).
        RandomBots::OnStartup();
        TC_LOG_INFO("server.loading", "Playerbots: MVP loaded. Enable={}, Tick={}ms", _enabled, _tickMs);
    }

    void Manager::ReloadConfig()
    {
        _enabled = sConfigMgr->GetBoolDefault("Playerbots.Enable", false);
        _tickMs = sConfigMgr->GetIntDefault("Playerbots.Tick", 200);
        if (_tickMs < 50)
            _tickMs = 50;

        SetFollowDistance(sConfigMgr->GetFloatDefault("Playerbots.FollowDistance", 10.0f));
        SetFollowAngle(sConfigMgr->GetFloatDefault("Playerbots.FollowAngle", 0.0f));

        _commandMinIntervalMs = sConfigMgr->GetIntDefault("Playerbots.CommandMinIntervalMs", 250);
        if (_commandMinIntervalMs < 0)
            _commandMinIntervalMs = 0;

        TC_LOG_INFO("server.loading",
            "Playerbots: Config reloaded. Enable={} Tick={}ms FollowDist={} FollowAngle={} CmdMinInterval={}ms",
            _enabled ? 1 : 0, _tickMs, _followDistance, _followAngle, _commandMinIntervalMs);
    }

    static AI::PlayerbotAI* GetOrCreateAI(ObjectGuid botGuid)
    {
        auto it = _aiByBot.find(botGuid);
        if (it != _aiByBot.end())
            return it->second.get();

        auto ai = std::make_unique<AI::PlayerbotAI>();
        AI::PlayerbotAI* ptr = ai.get();
        _aiByBot.emplace(botGuid, std::move(ai));
        return ptr;
    }

    void Manager::Update(uint32 diff)
    {
        if (!_enabled)
            return;

        _accumulated += diff;
        if (_accumulated < _tickMs)
            return;
        uint32 elapsed = _accumulated;
        _accumulated = 0;

        auto const& entries = Registry::Instance().GetEntries();

        // Opportunistic garbage collection:
        // - Drop cached formations for leaders that are no longer referenced.
        // - Drop cached AI state for bots that are no longer registered.
        // This keeps runtime state bounded even if bots are cycled aggressively.
        _gcAccumulated += elapsed;
        if (_gcAccumulated >= 30 * IN_MILLISECONDS)
        {
            _gcAccumulated = 0;

            std::unordered_set<ObjectGuid> activeBots;
            std::unordered_set<ObjectGuid> activeLeaders;
            activeBots.reserve(entries.size());
            activeLeaders.reserve(entries.size());

            for (auto const& [botGuid, entry] : entries)
            {
                activeBots.insert(botGuid);
                activeLeaders.insert(entry.LeaderGuid);
            }

            for (auto it = _aiByBot.begin(); it != _aiByBot.end();)
            {
                if (activeBots.find(it->first) == activeBots.end())
                    it = _aiByBot.erase(it);
                else
                    ++it;
            }

            for (auto it = _formationByLeader.begin(); it != _formationByLeader.end();)
            {
                if (activeLeaders.find(it->first) == activeLeaders.end())
                    it = _formationByLeader.erase(it);
                else
                    ++it;
            }
        }
        for (auto const& [botGuid, entry] : entries)
        {
            if (entry.Paused)
                continue;

            // Tick headless bot session + player update (World won't do it for socket-less sessions).
            // IMPORTANT: session update may detach/cleanup the player (logout path),
            // so we only call Player::Update if the session is still bound to the same player.
            Player* botPlayer = ObjectAccessor::FindPlayer(botGuid);
            if (botPlayer)
            {
                if (WorldSession* botSession = botPlayer->GetSession())
                {
                    WorldSessionFilter filter(botSession);
                    botSession->Update(elapsed, filter);

                    // Session update may have logged out / detached the player.
                    if (botSession->GetPlayer() != botPlayer)
                        continue;
                }

                // Player update ticks auras/cooldowns/movement timers etc.
                botPlayer->Update(elapsed);
            }

            Player* leader = ObjectAccessor::FindPlayer(entry.LeaderGuid);
            if (!leader)
                continue;

            if (!botPlayer)
                continue;

            if (!botPlayer->IsAlive() || botPlayer->GetMap() != leader->GetMap())
                continue;

            // AI first: it can switch to COMBAT and take over movement (chase target).
            AI::PlayerbotAI* ai = GetOrCreateAI(botGuid);
            if (ai)
                ai->Update(botPlayer, leader, elapsed);

            // In combat: do NOT apply follow distances/formation logic, AI handles chase.
            if (ai && ai->IsInCombat())
                continue;

            float dist = botPlayer->GetDistance(leader);
            float stopDist = _followDistance * _stopDistanceFactor;

            // Too far => ensure follow
            if (dist > _followDistance)
            {
                if (!entry.Following)
                    Registry::Instance().SetFollowing(botGuid, true);

                // Stable per-leader index: build it from sorted GUIDs (MVP, few units).
                uint32 idx = 0;
                uint32 total = 1;
                {
                    // gather all bots for this leader and sort by guid to have deterministic indices
                    std::vector<ObjectGuid> group;
                    group.reserve(8);
                    for (auto const& [g, e] : entries)
                        if (e.LeaderGuid == entry.LeaderGuid)
                            group.push_back(g);
                    std::sort(group.begin(), group.end());
                    total = uint32(group.size());
                    auto it = std::find(group.begin(), group.end(), botGuid);
                    if (it != group.end())
                        idx = uint32(std::distance(group.begin(), it));
                }

                Formation::Type formation = GetFormationFor(entry.LeaderGuid);
                Formation::FollowParams fp = Formation::Compute(formation, 2.0f, _followAngle, idx, total, botGuid);
                Movement::Follow(botPlayer, leader, fp.Dist, fp.Angle);
                continue;
            }

            // Close enough => stop following (stay where it is)
            if (dist <= stopDist && entry.Following)
            {
                Movement::Stop(botPlayer);
                Registry::Instance().SetFollowing(botGuid, false);
            }
        }

        TC_LOG_DEBUG("playerbots", "Playerbots: tick ({}ms) entries={}", _tickMs, uint32(entries.size()));
    }
}