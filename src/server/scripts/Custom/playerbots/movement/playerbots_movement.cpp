/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots_movement.h"

#include "MotionMaster.h"
#include "Player.h"
#include "Unit.h"

namespace Playerbots::Movement
{
    void Follow(Unit* follower, Player* leader, float dist, float angle)
    {
        if (!follower || !leader || follower == leader)
            return;

        follower->GetMotionMaster()->MoveFollow(leader, dist, angle);
    }

    void Unfollow(Unit* unit)
    {
        if (!unit)
            return;

        unit->StopMoving();
        unit->GetMotionMaster()->Clear(MOTION_SLOT_ACTIVE);

        if (unit->IsCreature())
            unit->GetMotionMaster()->MoveTargetedHome();
        else
            unit->GetMotionMaster()->MoveIdle();
    }

    void Stop(Unit* unit)
    {
        if (!unit)
            return;

        unit->StopMoving();
        unit->GetMotionMaster()->Clear(MOTION_SLOT_ACTIVE);
        unit->GetMotionMaster()->MoveIdle();
    }
}