/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_SPAWNER_H
#define PLAYERBOTS_SPAWNER_H

#include "ObjectGuid.h"

class Player;

namespace Playerbots
{
    class Spawner final
    {
    public:
        // Spawns the given bot character into the world.
        // Returns the spawned Player* on success.
        static Player* SpawnByGuid(Player* requester, ObjectGuid botGuid, bool reserveForRequester);

        // Despawns a headless bot spawned via this module.
        // This is a "clean logout" replacement for World::RemoveActiveSession which is not used for headless bots.
        static bool DespawnBot(Player* bot, bool saveToDB);

        // Picks one free random bot (offline + owner_guid=0) and spawns it.
        static Player* SpawnRandom(Player* requester, bool reserveForRequester);

    private:
        static Player* SpawnInternal(Player* requester, ObjectGuid botGuid, bool reserveForRequester);
    };
}

#endif // PLAYERBOTS_SPAWNER_H