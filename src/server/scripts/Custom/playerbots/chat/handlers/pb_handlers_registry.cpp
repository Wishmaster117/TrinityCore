/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots/chat/handlers/pb_handler.h"

namespace Playerbots::PB
{
    // These functions are defined in the handler units below.
    void RegisterFollowHandlers();
    void RegisterFormationHandlers();
    void RegisterJoinHandlers();
    void RegisterAdminHandlers();

    void RegisterAllHandlers()
    {
        RegisterFollowHandlers();
        RegisterFormationHandlers();
        RegisterJoinHandlers();
        RegisterAdminHandlers();
    }
}