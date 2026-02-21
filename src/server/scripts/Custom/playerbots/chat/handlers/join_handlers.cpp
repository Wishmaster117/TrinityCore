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
#include "Log.h"
#include "Player.h"
#include "WorldSession.h"

#include "playerbots/core/playerbots_grouping.h"
#include "playerbots/movement/playerbots_movement.h"

namespace Playerbots::PB
{
    std::unordered_map<std::string, std::function<HandlerResult(Command const&, Player*, Player*)>>& WhisperMap();

    static HandlerResult DoJoin(Command const& /*cmd*/, Player* master, Player* bot)
    {
        if (!master || !bot)
            return HandlerResult::HandledError;

        if (EnsureGroupedAndOwned(bot, master))
        {
            // Hard proof the code path is reached even if loggers are filtered.
            ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: %s joined your group.", bot->GetName().c_str());

             // Teleport (best-effort). If it fails, logs will tell why.
             bool tpOk = Playerbots::Movement::TeleportNear(bot, master);
             if (tpOk)
                ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: %s teleported to you.", bot->GetName().c_str());
            else
                ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: teleport of %s failed.", bot->GetName().c_str());
 
             TC_LOG_INFO("playerbots", "Playerbots: join command handled: master={} bot={} tpOk={}",
                 master->GetGUID().ToString(), bot->GetGUID().ToString(), tpOk);

            return HandlerResult::HandledOk;
        }

        ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: %s could not join your group.", bot->GetName().c_str());
        return HandlerResult::HandledError;
    }

    void RegisterJoinHandlers()
    {
        WhisperMap()["join"] = &DoJoin;
    }
}