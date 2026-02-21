/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots_bot_store.h"

#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"

namespace Playerbots
{
    BotStore& BotStore::Instance()
    {
        static BotStore instance;
        return instance;
    }

    void BotStore::EnsureLoaded()
    {
        if (_loaded)
            return;
        LoadInternal();
    }

    void BotStore::Reload()
    {
        _loaded = false;
        _botsByGuidLow.clear();
        LoadInternal();
    }

    bool BotStore::IsBot(ObjectGuid guid) const
    {
        if (!guid)
            return false;

        uint64 low = guid.GetCounter();
        return _botsByGuidLow.find(low) != _botsByGuidLow.end();
    }

    bool BotStore::IsBot(Player const* player) const
    {
        if (!player)
            return false;
        return IsBot(player->GetGUID());
    }

    BotDbEntry const* BotStore::Get(ObjectGuid guid) const
    {
        if (!guid)
            return nullptr;

        uint64 low = guid.GetCounter();
        auto it = _botsByGuidLow.find(low);
        if (it == _botsByGuidLow.end())
            return nullptr;
        return &it->second;
    }

    uint64 BotStore::GetOwnerLow(ObjectGuid guid) const
    {
        if (BotDbEntry const* e = Get(guid))
            return e->OwnerGuidLow;
        return 0;
    }

    BotType BotStore::GetType(ObjectGuid guid) const
    {
        if (BotDbEntry const* e = Get(guid))
            return e->Type;
        return BotType::SelfBot;
    }

    bool BotStore::SetOwnerLow(ObjectGuid botGuid, uint64 ownerGuidLow)
    {
        if (!botGuid)
            return false;

        uint64 botLow = botGuid.GetCounter();
        auto it = _botsByGuidLow.find(botLow);
        if (it == _botsByGuidLow.end())
            return false;

        // Persist using raw SQL (numeric only). This avoids requiring core DB statement enums.
        std::string sql = "UPDATE playerbots_bots SET owner_guid = " + std::to_string(ownerGuidLow) +
                          " WHERE guid = " + std::to_string(botLow);
        CharacterDatabase.DirectExecute(sql.c_str());

        // Update cache
        it->second.OwnerGuidLow = ownerGuidLow;
        return true;
    }

    void BotStore::LoadInternal()
    {
        // Lazy-load cache from Characters DB.
        // Table is created by provided SQL: characters.playerbots_bots
        QueryResult result = CharacterDatabase.Query("SELECT guid, bot_type, owner_guid FROM playerbots_bots");
        if (!result)
        {
            _loaded = true;
            TC_LOG_INFO("server.loading", "Playerbots: BotStore loaded (0 bots).");
            return;
        }

        uint32 count = 0;
        do
        {
            Field* fields = result->Fetch();
            uint64 guidLow = fields[0].GetUInt64();
            uint8 typeU8 = fields[1].GetUInt8();
            uint64 ownerLow = fields[2].GetUInt64();

            BotDbEntry entry;
            entry.Type = static_cast<BotType>(typeU8);
            entry.OwnerGuidLow = ownerLow;
            _botsByGuidLow[guidLow] = entry;
            ++count;
        } while (result->NextRow());

        _loaded = true;
        TC_LOG_INFO("server.loading", "Playerbots: BotStore loaded ({} bots).", count);
    }
}