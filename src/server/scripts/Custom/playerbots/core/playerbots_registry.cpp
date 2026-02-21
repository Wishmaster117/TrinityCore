/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots_registry.h"
#include "Player.h"
#include "Unit.h"

namespace Playerbots
{
    Registry& Registry::Instance()
    {
        static Registry instance;
        return instance;
    }

    bool Registry::Add(Unit* unit, Player* leader)
    {
        if (!unit || !leader)
            return false;

        RegistryEntry entry;
        entry.LeaderGuid = leader->GetGUID();
        entry.Paused = false;
        entry.Following = false;

        auto [it, inserted] = _entries.emplace(unit->GetGUID(), entry);
        if (!inserted)
        {
            it->second.LeaderGuid = leader->GetGUID();
            it->second.Paused = false;
            // Keep Following as-is to preserve current mode across leader changes.
        }

        return inserted;
    }

    bool Registry::Remove(Unit* unit)
    {
        if (!unit)
            return false;

        return _entries.erase(unit->GetGUID()) > 0;
    }

    bool Registry::Contains(Unit* unit) const
    {
        if (!unit)
            return false;

        return _entries.find(unit->GetGUID()) != _entries.end();
    }

    void Registry::Clear()
    {
        _entries.clear();
    }

    RegistryEntry const* Registry::GetEntry(ObjectGuid botGuid) const
    {
        auto it = _entries.find(botGuid);
        if (it == _entries.end())
            return nullptr;
        return &it->second;
    }

    RegistryEntry* Registry::GetEntry(ObjectGuid botGuid)
    {
        auto it = _entries.find(botGuid);
        if (it == _entries.end())
            return nullptr;
        return &it->second;
    }

    bool Registry::SetPaused(Unit* unit, bool paused)
    {
        if (!unit)
            return false;

        auto it = _entries.find(unit->GetGUID());
        if (it == _entries.end())
            return false;

        it->second.Paused = paused;
        return true;
    }

    bool Registry::SetPaused(ObjectGuid botGuid, bool paused)
    {
        auto it = _entries.find(botGuid);
        if (it == _entries.end())
            return false;

        it->second.Paused = paused;
        return true;
    }


    bool Registry::SetFollowing(Unit* unit, bool following)
    {
        if (!unit)
            return false;

        return SetFollowing(unit->GetGUID(), following);
    }

    bool Registry::SetFollowing(ObjectGuid botGuid, bool following)
    {
        auto it = _entries.find(botGuid);
        if (it == _entries.end())
            return false;

        it->second.Following = following;
        return true;
    }

    bool Registry::SetLeaderGuid(ObjectGuid botGuid, ObjectGuid leaderGuid)
    {
        auto it = _entries.find(botGuid);
        if (it == _entries.end())
            return false;

        it->second.LeaderGuid = leaderGuid;
        return true;
    }
}