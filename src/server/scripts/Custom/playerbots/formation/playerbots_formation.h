/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_FORMATION_H
#define PLAYERBOTS_FORMATION_H

#include "Define.h"

class ObjectGuid;

namespace Playerbots::Formation
{
    enum class Type : uint8
    {
        Arrow,
        Queue,
        Near,
        Melee,
        Line,
        Circle,
        Chaos,
        Shield
    };

    struct FollowParams
    {
        float Dist = 2.0f;
        // Angle is in TC MoveFollow convention:
        // 0 = behind the leader, PI = in front of the leader.
        // If you use WorldObject::GetClosePoint, convert via (Angle + PI).
        float Angle = 0.0f;
    };

    char const* ToString(Type type);

    // Compute follow params for a bot in a leader group.
    // - baseDist/baseAngle come from your existing settings.
    // - index/total define stable placement within the group.
    // - botGuid is used to create stable pseudo-randomness for Chaos/Near variants.
    FollowParams Compute(Type type, float baseDist, float baseAngle, uint32 index, uint32 total, ObjectGuid const& botGuid);
}

#endif // PLAYERBOTS_FORMATION_H