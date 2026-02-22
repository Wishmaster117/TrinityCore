/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_PB_HANDLER_H
#define PLAYERBOTS_PB_HANDLER_H

#include "Define.h"
#include "playerbots/chat/command_parser.h"

class Player;
class Group;

namespace Playerbots::PB
{
    enum class HandlerResult : uint8
    {
        NotHandled = 0,
        HandledOk  = 1,
        HandledError = 2
    };

    // Registration entry point.
    void RegisterAllHandlers();

    // No-prefix support: only react to messages that match an existing bot command.
    bool IsKnownCommand(std::string const& name);

    // Dispatching API used by CommandDispatcher.
    HandlerResult DispatchToWhisperedBot(Command const& cmd, Player* master, Player* bot);
    HandlerResult DispatchToGroupBot(Command const& cmd, Player* master, Player* bot);
    HandlerResult DispatchLeaderOnlyGroupCommand(Command const& cmd, Player* master, Group* group);
}

#endif // PLAYERBOTS_PB_HANDLER_H