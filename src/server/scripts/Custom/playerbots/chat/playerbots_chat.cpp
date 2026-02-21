/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"
#include "Log.h"
#include "Group.h"
#include "WorldSession.h"

#include "playerbots/chat/command_parser.h"
#include "playerbots/chat/command_dispatcher.h"
#include "playerbots/chat/handlers/pb_handler.h"

#include <mutex>

namespace
{
    static void HandlePlayerbotsChat(Player* player, uint32 type, std::string& msg, Player* receiver)
    {
        if (!player || msg.empty())
            return;

        Playerbots::Command cmd;
        if (!Playerbots::CommandParser::TryParse(msg, cmd))
            return;

        // No-prefix support:
        // do not hijack normal chat unless the first token matches a registered bot command.
        if (!Playerbots::PB::IsKnownCommand(cmd.Name))
            return;

        // 1) Whisper-to-bot (direct).
        if (receiver && type == CHAT_MSG_WHISPER)
        {
            Playerbots::CommandContext ctx;
            ctx.Master = player;
            ctx.Bot = receiver;
            ctx.Channel = Playerbots::ChatChannel::Whisper;
            Playerbots::CommandDispatcher::Instance().Dispatch(cmd, ctx);
            return;
        }

        // 2) Party/Raid/RW/Instance broadcast (only in these channels, and only if player is in a group).
        Playerbots::ChatChannel channel;
        switch (type)
        {
            case CHAT_MSG_PARTY:         channel = Playerbots::ChatChannel::Party; break;
            case CHAT_MSG_RAID:          channel = Playerbots::ChatChannel::Raid; break;
            case CHAT_MSG_RAID_WARNING:  channel = Playerbots::ChatChannel::RaidWarning; break;
            case CHAT_MSG_INSTANCE_CHAT: channel = Playerbots::ChatChannel::Instance; break;
            default:
                return; // do not treat /say /yell /guild /etc
        }

        if (Group* group = player->GetGroup())
        {
            Playerbots::CommandContext ctx;
            ctx.Master = player;
            ctx.Bot = nullptr;
            ctx.Channel = channel;
            ctx.Group = group;
            Playerbots::CommandDispatcher::Instance().Dispatch(cmd, ctx);
        }
    }
}

// NOTE:
// The exact OnChat signature can differ between TrinityCore revisions.
// If this doesn't compile, paste the compile error and I'll adjust the signature to your TC master.
class PlayerbotsChatScript final : public PlayerScript
{
public:
    PlayerbotsChatScript() : PlayerScript("PlayerbotsChatScript") { }

    // Some TrinityCore revisions call this overload for group/party chat.
    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg) override
    {
        HandlePlayerbotsChat(player, CHAT_MSG_PARTY, msg, nullptr);
    }

    // Some TrinityCore revisions call this overload for whispers (receiver != nullptr),
    // and sometimes also for other chat types. We support both.
    void OnChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg, Player* receiver) override
    {
        HandlePlayerbotsChat(player, type, msg, receiver);
    }
};

void AddSC_playerbots_chat()
{
    // Register handlers once (single source of truth for IsKnownCommand + dispatch).
    // This must happen before any chat is parsed, otherwise IsKnownCommand() returns false and bots ignore commands.
    static std::once_flag s_once;
    std::call_once(s_once, []()
    {
        Playerbots::PB::RegisterAllHandlers();
    });

    TC_LOG_INFO("server.loading", "Playerbots: registering chat script (PlayerbotsChatScript).");
    new PlayerbotsChatScript();
}