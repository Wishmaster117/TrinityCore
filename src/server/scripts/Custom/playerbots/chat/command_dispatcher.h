/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_COMMAND_DISPATCHER_H
#define PLAYERBOTS_COMMAND_DISPATCHER_H

#include "playerbots/chat/command_parser.h"

#include "ObjectGuid.h"

class Player;
class Group;

namespace Playerbots
{
    enum class ChatChannel : uint8
    {
        Whisper      = 0,
        Party        = 1,
        Raid         = 2,
        RaidWarning  = 3,
        Instance     = 4
    };

    struct CommandContext
    {
        Player* Master = nullptr;
        Player* Bot = nullptr;      // only for Whisper, or per-bot iteration on Group dispatch
        Group* Group = nullptr;     // only for Group channel
        ChatChannel Channel = ChatChannel::Whisper;
    };

    class CommandDispatcher final
    {
    public:
        static CommandDispatcher& Instance();

        void Dispatch(Command const& cmd, CommandContext& ctx);

        CommandDispatcher();

        bool DispatchWhisper(Command const& cmd, CommandContext& ctx);
        void DispatchBroadcast(Command const& cmd, CommandContext& ctx);
    };
}

#endif // PLAYERBOTS_COMMAND_DISPATCHER_H