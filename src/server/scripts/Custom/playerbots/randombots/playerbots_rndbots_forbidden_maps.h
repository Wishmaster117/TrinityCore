/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_RNDBOTS_FORBIDDEN_MAPS_H
#define PLAYERBOTS_RNDBOTS_FORBIDDEN_MAPS_H

#include <string>
#include <unordered_set>

namespace Playerbots::RandomBots
{
    struct ForbiddenMaps
    {
        // Parsed set of map IDs.
        std::unordered_set<uint32> Ids;

        // Pre-built "1,530,571" list (digits only) that can be injected into SQL.
        // Empty if Ids is empty.
        std::string NotInSql;

        bool IsForbidden(uint32 mapId) const
        {
            return !Ids.empty() && (Ids.find(mapId) != Ids.end());
        }

        static ForbiddenMaps Parse(std::string const& text)
        {
            ForbiddenMaps out;

            uint32 value = 0;
            bool inNumber = false;

            auto flush = [&]()
            {
                if (!inNumber)
                    return;
                out.Ids.insert(value);
                value = 0;
                inNumber = false;
            };

            // Accept separators: ',', ';', whitespace, and any non-digit.
            for (char c : text)
            {
                if (c >= '0' && c <= '9')
                {
                    inNumber = true;
                    value = value * 10u + uint32(c - '0');
                }
                else
                    flush();
            }
            flush();

            if (!out.Ids.empty())
            {
                // Build a stable list (order doesn't matter for correctness).
                // Keep it simple: iterate the set.
                bool first = true;
                out.NotInSql.reserve(out.Ids.size() * 6);
                for (uint32 id : out.Ids)
                {
                    if (!first)
                        out.NotInSql += ",";
                    first = false;
                    out.NotInSql += std::to_string(id);
                }
            }

            return out;
        }
    };
}

#endif // PLAYERBOTS_RNDBOTS_FORBIDDEN_MAPS_H