/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots/chat/handlers/pb_handler.h"

#include "Chat.h"
#include "Player.h"
#include "WorldSession.h"

#include "playerbots/core/playerbots_registry.h"
#include "playerbots/movement/playerbots_movement.h"

namespace Playerbots::PB
{
    // From pb_handler.cpp (internal maps accessors)
    std::unordered_map<std::string, std::function<HandlerResult(Command const&, Player*, Player*)>>& WhisperMap();
    std::unordered_map<std::string, std::function<HandlerResult(Command const&, Player*, Player*)>>& GroupBotMap();

    static HandlerResult DoFollow(Command const& /*cmd*/, Player* master, Player* bot)
    {
        Registry::Instance().SetPaused(bot, false);
        Registry::Instance().SetFollowing(bot, true);
        Movement::Follow(bot, master);
        ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: %s -> follow.", bot->GetName().c_str());
        return HandlerResult::HandledOk;
    }

    static HandlerResult DoUnfollow(Command const& /*cmd*/, Player* master, Player* bot)
    {
        Registry::Instance().SetFollowing(bot, false);
        Registry::Instance().SetPaused(bot, false);
        Movement::Unfollow(bot);
        ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: %s -> unfollow.", bot->GetName().c_str());
        return HandlerResult::HandledOk;
    }

    static HandlerResult DoStop(Command const& /*cmd*/, Player* master, Player* bot)
    {
        Movement::Stop(bot);
        Registry::Instance().SetPaused(bot, true);
        Registry::Instance().SetFollowing(bot, false);
        ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: %s -> stop.", bot->GetName().c_str());
        return HandlerResult::HandledOk;
    }

    static HandlerResult DoResume(Command const& /*cmd*/, Player* master, Player* bot)
    {
        Registry::Instance().SetPaused(bot, false);
        ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: %s -> resume.", bot->GetName().c_str());
        return HandlerResult::HandledOk;
    }

    void RegisterFollowHandlers()
    {
        WhisperMap()["follow"]   = &DoFollow;
        WhisperMap()["unfollow"] = &DoUnfollow;
        WhisperMap()["stop"]     = &DoStop;
        WhisperMap()["resume"]   = &DoResume;

        GroupBotMap()["follow"]   = &DoFollow;
        GroupBotMap()["unfollow"] = &DoUnfollow;
        GroupBotMap()["stop"]     = &DoStop;
        GroupBotMap()["resume"]   = &DoResume;
    }
}