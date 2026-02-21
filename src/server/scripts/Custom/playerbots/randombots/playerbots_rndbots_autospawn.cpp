/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"

#include "Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectGuid.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "World.h"

#include "playerbots/core/playerbots_spawner.h"
#include "playerbots/core/playerbots_bot_store.h"

#include <vector>

namespace Playerbots::RandomBots
{
    namespace
    {
        class RandomBotsAutoSpawnWorldScript final : public WorldScript
        {
        public:
            RandomBotsAutoSpawnWorldScript() : WorldScript("RandomBotsAutoSpawnWorldScript") { }

            void OnStartup() override
            {
                _enabled = sConfigMgr->GetBoolDefault("Playerbots.RandomBots.AutoSpawnEnable", true);
                if (!_enabled)
                    return;

                // Crash recovery:
                // If the server crashed, bots may still be flagged online=1 in DB, which would block future spawns
                // (IsOffline check). Reset online flags and release ownership for random bots.
                CharacterDatabase.DirectExecute(
                    "UPDATE characters c "
                    "JOIN playerbots_bots b ON b.guid=c.guid "
                    "SET c.online=0 "
                    "WHERE c.online<>0");

                CharacterDatabase.DirectExecute(
                    "UPDATE playerbots_bots "
                    "SET owner_guid=0 "
                    "WHERE owner_guid<>0");

                _delayMs = sConfigMgr->GetIntDefault("Playerbots.RandomBots.AutoSpawnDelayMs", 2000);
                if (_delayMs < 0)
                    _delayMs = 0;

                _batchPerTick = sConfigMgr->GetIntDefault("Playerbots.RandomBots.AutoSpawnBatchPerTick", 10);
                if (_batchPerTick < 1)
                    _batchPerTick = 1;

                _target = sConfigMgr->GetIntDefault("Playerbots.RandomBots.MaxRandomBots", 0);
                if (_target == 0)
                {
                    TC_LOG_INFO("server.loading", "Playerbots.RandomBots: AutoSpawn enabled but MaxRandomBots=0, skipping.");
                    _enabled = false;
                    return;
                }

                // Safety recovery: if the server crashed previously, bots might be stuck as online in DB.
                // Reset only our random bots.
                CharacterDatabase.DirectExecute(
                    "UPDATE characters c "
                    "JOIN playerbots_bots b ON b.guid=c.guid "
                    "SET c.online=0 "
                    "WHERE b.bot_type=1 AND c.online<>0");

                // Release ownership reservations for random bots on startup (headless pool).
                CharacterDatabase.DirectExecute("UPDATE playerbots_bots SET owner_guid=0 WHERE bot_type=1 AND owner_guid<>0");

                BuildSpawnQueue();

                TC_LOG_INFO("server.loading",
                    "Playerbots.RandomBots: AutoSpawn prepared. target={} queued={} delayMs={} batchPerTick={}",
                    _target, uint32(_queue.size()), _delayMs, _batchPerTick);
            }

            void OnShutdown() override
            {
                // Always clean up pool state so bots remain available after restart/crash.
                // We do best-effort despawn of in-memory bots and then hard reset DB flags.

                // 1) Despawn all online headless bots that belong to playerbots_bots bot_type=1
                // Collect first to avoid iterator invalidation while removing from world.
                std::vector<ObjectGuid> toDespawn;
                toDespawn.reserve(1024);

                BotStore::Instance().EnsureLoaded();

                // TrinityCore: ObjectAccessor::GetPlayers() returns a container of active players.
                // We only despawn players that are bots managed by this module.
                auto const& players = ObjectAccessor::GetPlayers();
                for (auto const& pair : players)
                {
                    Player* p = pair.second;
                    if (!p)
                        continue;

                    ObjectGuid guid = p->GetGUID();
                    if (!guid.IsPlayer())
                        continue;

                    if (!BotStore::Instance().IsBot(guid))
                        continue;

                    toDespawn.push_back(guid);
                }

                uint32 despawned = 0;
                for (ObjectGuid const& guid : toDespawn)
                {
                    Player* bot = ObjectAccessor::FindPlayer(guid);
                    if (!bot)
                        continue;

                    // No heavy DB save on shutdown by default; we mainly need online=0 and a clean teardown.
                    if (Spawner::DespawnBot(bot, /*saveToDB=*/false))
                        ++despawned;
                }

                // 2) Hard reset DB flags for robustness (crash-safe).
                // Ensure bots are not stuck online and not reserved by someone.
                CharacterDatabase.DirectExecute(
                    "UPDATE characters c "
                    "JOIN playerbots_bots b ON b.guid=c.guid "
                    "SET c.online=0 "
                    "WHERE b.bot_type=1 AND c.online<>0");

                CharacterDatabase.DirectExecute(
                    "UPDATE playerbots_bots SET owner_guid=0 "
                    "WHERE bot_type=1 AND owner_guid<>0");

                TC_LOG_INFO("server.loading", "Playerbots.RandomBots: shutdown cleanup complete. despawnedInMemory={}", despawned);
            }

            void OnUpdate(uint32 diff) override
            {
                if (!_enabled)
                    return;

                // Wait a bit after startup (maps loaded, scripts ready).
                if (_delayMs > 0)
                {
                    if (diff >= uint32(_delayMs))
                        _delayMs = 0;
                    else
                    {
                        _delayMs -= int32(diff);
                        return;
                    }
                }

                if (_index >= _queue.size())
                    return;

                uint32 spawnedThisTick = 0;
                while (_index < _queue.size() && spawnedThisTick < uint32(_batchPerTick))
                {
                    ObjectGuid botGuid = _queue[_index++];

                    // Spawn without requester (auto), no reservation.
                    Player* bot = Spawner::SpawnByGuid(nullptr, botGuid, /*reserveForRequester=*/false);
                    if (!bot)
                    {
                        // Keep going; bot could be temporarily invalid, or already spawned by someone else.
                        continue;
                    }

                    ++spawnedThisTick;
                }

                if (_index >= _queue.size())
                {
                    TC_LOG_INFO("server.loading", "Playerbots.RandomBots: AutoSpawn complete. spawnedTarget={}", uint32(_queue.size()));
                }
            }

        private:
            void BuildSpawnQueue()
            {
                _queue.clear();
                _queue.reserve(_target);

                // Select random-bot guids that are offline.
                // We rely on characters position fields already set by bootstrap:
                // - level 1: normal spawn or Exile's Reach via createMode/NPE
                // - level > 1: spawn hubs already applied into characters row
                QueryResult r = CharacterDatabase.Query(fmt::format(
                    "SELECT b.guid "
                    "FROM playerbots_bots b "
                    "JOIN characters c ON c.guid=b.guid "
                    "WHERE b.bot_type=1 AND b.owner_guid=0 AND c.online=0 "
                    "ORDER BY b.guid ASC "
                    "LIMIT {}",
                    _target).c_str());

                if (!r)
                    return;

                do
                {
                    uint32 guidLow = r->Fetch()[0].GetUInt32();
                    _queue.push_back(ObjectGuid::Create<HighGuid::Player>(guidLow));
                } while (r->NextRow());
            }

            bool _enabled = false;
            int32 _delayMs = 0;
            int32 _batchPerTick = 10;
            uint32 _target = 0;

            std::vector<ObjectGuid> _queue;
            size_t _index = 0;
        };
    }
}

void AddSC_playerbots_rndbots_autospawn()
{
    new Playerbots::RandomBots::RandomBotsAutoSpawnWorldScript();
}