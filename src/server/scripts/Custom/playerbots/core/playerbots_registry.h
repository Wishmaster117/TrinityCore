/*
+ * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
+ *
+ * This program is free software; you can redistribute it and/or modify it
+ * under the terms of the GNU General Public License as published by the
+ * Free Software Foundation; either version 2 of the License, or (at your
+ * option) any later version.
+ */

#ifndef PLAYERBOTS_REGISTRY_H
#define PLAYERBOTS_REGISTRY_H

#include "Define.h"
#include <unordered_map>

class Unit;
class Player;

namespace Playerbots
{
    struct RegistryEntry
    {
        ObjectGuid LeaderGuid;
        bool Paused = false;     // if true, Manager won't drive this unit
        bool Following = false;  // current intended mode for hysteresis logic
    };

    class Registry
    {
    public:
        static Registry& Instance();

        bool Add(Unit* unit, Player* leader);
        bool Remove(Unit* unit);
        bool Contains(Unit* unit) const;
        void Clear();

        // botGuid -> RegistryEntry
        std::unordered_map<ObjectGuid, RegistryEntry> const& GetEntries() const { return _entries; }

        RegistryEntry const* GetEntry(ObjectGuid botGuid) const;
        RegistryEntry* GetEntry(ObjectGuid botGuid);

        bool SetPaused(Unit* unit, bool paused);
        bool SetPaused(ObjectGuid botGuid, bool paused);

        bool SetFollowing(Unit* unit, bool following);
        bool SetFollowing(ObjectGuid botGuid, bool following);

        bool SetLeaderGuid(ObjectGuid botGuid, ObjectGuid leaderGuid);

    private:
        std::unordered_map<ObjectGuid, RegistryEntry> _entries;
    };
}

#endif // PLAYERBOTS_REGISTRY_H