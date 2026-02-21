/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_MANAGER_H
#define PLAYERBOTS_MANAGER_H

#include "Define.h"

namespace Playerbots::Formation { enum class Type : uint8; }

namespace Playerbots
{
    class Manager
    {
    public:
        static Manager& Instance();

        void OnStartup();
        void Update(uint32 diff);
        void ReloadConfig();

        bool IsEnabled() const;
        uint32 GetTickMs() const;
        void SetEnabled(bool enabled);
        void SetTickMs(uint32 tickMs);

        float GetFollowDistance() const;
        float GetFollowAngle() const;
        void SetFollowDistance(float dist);
        void SetFollowAngle(float angle);

        uint32 GetCommandMinIntervalMs() const;

        Formation::Type GetFormationFor(ObjectGuid leaderGuid) const;
        void SetFormationFor(ObjectGuid leaderGuid, Formation::Type type);
    };
}

#endif // PLAYERBOTS_MANAGER_H