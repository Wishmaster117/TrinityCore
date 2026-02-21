/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_GROUPING_H
#define PLAYERBOTS_GROUPING_H

class Player;

namespace Playerbots
{
    // Ensures:
    // - bot is DB-flagged
    // - bot is in master's group (creates group if needed)
    // - registry entry exists and is owned by master
    //
    // Returns false if bot is in another group or if group/add/register fails.
    bool EnsureGroupedAndOwned(Player* bot, Player* master);
}

#endif // PLAYERBOTS_GROUPING_H