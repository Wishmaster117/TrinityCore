/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_MOVEMENT_H
#define PLAYERBOTS_MOVEMENT_H

class Unit;
class Player;

namespace Playerbots
{
    namespace Movement
    {
        void Follow(Unit* follower, Player* leader, float dist = 2.0f, float angle = 0.0f);
        void Unfollow(Unit* unit);
        void Stop(Unit* unit);
    }
}

#endif // PLAYERBOTS_MOVEMENT_H