/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_BOT_STORE_H
#define PLAYERBOTS_BOT_STORE_H

#include "Define.h"
#include "ObjectGuid.h"

#include <unordered_map>

namespace Playerbots
{
    enum class BotType : uint8
    {
        SelfBot     = 0,
        RandomBot   = 1,
        AddClassBot = 2
    };

    struct BotDbEntry
    {
        BotType Type = BotType::SelfBot;
        uint64 OwnerGuidLow = 0; // optional, for later (0 = any/unknown)
    };

    class BotStore final
    {
    public:
        static BotStore& Instance();

        void EnsureLoaded();
        void Reload();

        bool IsBot(ObjectGuid guid) const;
        bool IsBot(Player const* player) const;

        BotDbEntry const* Get(ObjectGuid guid) const;

        // Convenience
        uint64 GetOwnerLow(ObjectGuid guid) const;
        BotType GetType(ObjectGuid guid) const;

        // Persist + update cache
        // Returns false if guid is not a bot in the store.
        bool SetOwnerLow(ObjectGuid botGuid, uint64 ownerGuidLow);

    private:
        void LoadInternal();

        bool _loaded = false;
        std::unordered_map<uint64, BotDbEntry> _botsByGuidLow; // guidLow -> entry
    };
}

#endif // PLAYERBOTS_BOT_STORE_H