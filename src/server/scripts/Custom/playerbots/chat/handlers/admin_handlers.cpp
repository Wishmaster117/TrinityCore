/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots/chat/handlers/pb_handler.h"

#include <cstdlib>
#include "Chat.h"
 #include "Group.h"
 #include "Player.h"
 #include "WorldSession.h"
 
 #include "playerbots/core/playerbots_bot_store.h"
 #include "playerbots/core/playerbots_manager.h"
 #include "playerbots/formation/playerbots_formation.h"
 
 namespace Playerbots::PB
 {
     std::unordered_map<std::string, std::function<HandlerResult(Command const&, Player*, Group*)>>& LeaderGroupMap();
     std::unordered_map<std::string, std::function<HandlerResult(Command const&, Player*, Player*)>>& WhisperMap();
 
     static bool RequireLeaderOrAssistantOrSolo(Player* master)
     {
         if (!master)
             return false;
 
         Group* group = master->GetGroup();
         if (!group)
             return true; // allow when not grouped (solo testing)
 
         ObjectGuid g = master->GetGUID();
         if (group->IsLeader(g) || group->IsAssistant(g))
             return true;
 
         if (master->GetSession())
             ChatHandler(master->GetSession()).SendSysMessage("Playerbots: you must be group leader or assistant for this command.");
         return false;
     }
 
     static HandlerResult DoHelp(Command const& /*cmd*/, Player* master, Player* /*botOrNull*/)
     {
         if (!master)
             return HandlerResult::HandledError;
 
         ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: commands:");
         ChatHandler(master->GetSession()).PSendSysMessage("  join (whisper to bot)");
         ChatHandler(master->GetSession()).PSendSysMessage("  follow | unfollow | stop | resume");
         ChatHandler(master->GetSession()).PSendSysMessage("  formation <arrow|queue|near|melee|line|circle|chaos|shield>");
         ChatHandler(master->GetSession()).PSendSysMessage("  reloadbots (group leader only)");
         ChatHandler(master->GetSession()).PSendSysMessage("  status (leader/assistant or solo)");
         ChatHandler(master->GetSession()).PSendSysMessage("  on | off (leader/assistant or solo)");
         ChatHandler(master->GetSession()).PSendSysMessage("  tick <ms> (leader/assistant or solo)");
         ChatHandler(master->GetSession()).PSendSysMessage("  followdist <yards> | followangle <deg> (leader/assistant or solo)");
         return HandlerResult::HandledOk;
     }
 
     static HandlerResult DoStatus(Command const& /*cmd*/, Player* master, Group* /*group*/)
     {
         if (!master || !master->GetSession())
             return HandlerResult::HandledError;
 
         if (!RequireLeaderOrAssistantOrSolo(master))
             return HandlerResult::HandledError;
 
         auto& mgr = Playerbots::Manager::Instance();
         ChatHandler(master->GetSession()).PSendSysMessage(
             "Playerbots: Enable=%u Tick=%ums FollowDist=%.2f FollowAngle=%.2f CmdMinInterval=%ums",
             mgr.IsEnabled() ? 1 : 0, mgr.GetTickMs(), mgr.GetFollowDistance(), mgr.GetFollowAngle(), mgr.GetCommandMinIntervalMs());
 
         ChatHandler(master->GetSession()).PSendSysMessage(
             "Playerbots: Formation=%s", Playerbots::Formation::ToString(mgr.GetFormationFor(master->GetGUID())));
         return HandlerResult::HandledOk;
     }
 
     static HandlerResult DoOn(Command const& /*cmd*/, Player* master, Group* /*group*/)
     {
         if (!master || !master->GetSession())
             return HandlerResult::HandledError;
         if (!RequireLeaderOrAssistantOrSolo(master))
             return HandlerResult::HandledError;
 
         Playerbots::Manager::Instance().SetEnabled(true);
         ChatHandler(master->GetSession()).SendSysMessage("Playerbots: enabled.");
         return HandlerResult::HandledOk;
     }
 
     static HandlerResult DoOff(Command const& /*cmd*/, Player* master, Group* /*group*/)
     {
         if (!master || !master->GetSession())
             return HandlerResult::HandledError;
         if (!RequireLeaderOrAssistantOrSolo(master))
             return HandlerResult::HandledError;
 
         Playerbots::Manager::Instance().SetEnabled(false);
         ChatHandler(master->GetSession()).SendSysMessage("Playerbots: disabled.");
         return HandlerResult::HandledOk;
     }
 
     static HandlerResult DoTick(Command const& cmd, Player* master, Group* /*group*/)
     {
         if (!master || !master->GetSession())
             return HandlerResult::HandledError;
         if (!RequireLeaderOrAssistantOrSolo(master))
             return HandlerResult::HandledError;
 
         if (cmd.Args.size() < 2)
         {
             ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: usage: tick <ms>.");
             return HandlerResult::HandledError;
         }
 
         uint32 tickMs = static_cast<uint32>(std::max(0, atoi(cmd.Args[1].c_str())));
         Playerbots::Manager::Instance().SetTickMs(tickMs);
         ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: Tick=%ums.", Playerbots::Manager::Instance().GetTickMs());
         return HandlerResult::HandledOk;
     }
 
     static HandlerResult DoFollowDist(Command const& cmd, Player* master, Group* /*group*/)
     {
         if (!master || !master->GetSession())
             return HandlerResult::HandledError;
         if (!RequireLeaderOrAssistantOrSolo(master))
             return HandlerResult::HandledError;
 
         if (cmd.Args.size() < 2)
         {
             ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: usage: followdist <yards>.");
             return HandlerResult::HandledError;
         }
 
         float dist = static_cast<float>(atof(cmd.Args[1].c_str()));
         Playerbots::Manager::Instance().SetFollowDistance(dist);
         ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: FollowDist=%.2f.", Playerbots::Manager::Instance().GetFollowDistance());
         return HandlerResult::HandledOk;
     }
 
     static HandlerResult DoFollowAngle(Command const& cmd, Player* master, Group* /*group*/)
     {
         if (!master || !master->GetSession())
             return HandlerResult::HandledError;
         if (!RequireLeaderOrAssistantOrSolo(master))
             return HandlerResult::HandledError;
 
         if (cmd.Args.size() < 2)
         {
             ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: usage: followangle <deg>.");
             return HandlerResult::HandledError;
         }
 
         float angle = static_cast<float>(atof(cmd.Args[1].c_str()));
         Playerbots::Manager::Instance().SetFollowAngle(angle);
         ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: FollowAngle=%.2f.", Playerbots::Manager::Instance().GetFollowAngle());
         return HandlerResult::HandledOk;
     }

    static HandlerResult DoReloadBots(Command const& /*cmd*/, Player* master, Group* group)
    {
        if (!master || !group)
            return HandlerResult::HandledError;

        // Strict leader-only (not assistant).
        if (!group->IsLeader(master->GetGUID()))
            return HandlerResult::HandledError;

        BotStore::Instance().Reload();
        ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: bot store reloaded.");
        return HandlerResult::HandledOk;
    }

    void RegisterAdminHandlers()
    {
        // help is useful both via whisper and via group chat.
        WhisperMap()["help"] = &DoHelp;
        LeaderGroupMap()["help"] = [](Command const& cmd, Player* master, Group* /*group*/)
        {
            return DoHelp(cmd, master, nullptr);
        };

        // Leader/assistant (or solo) runtime tuning.
        LeaderGroupMap()["status"] = &DoStatus;
        LeaderGroupMap()["on"] = &DoOn;
        LeaderGroupMap()["off"] = &DoOff;
        LeaderGroupMap()["tick"] = &DoTick;
        LeaderGroupMap()["followdist"] = &DoFollowDist;
        LeaderGroupMap()["followangle"] = &DoFollowAngle;

        // Also allow these commands via whisper (owner-only path).
        // This avoids "silent no-op" when testing without using /party,
        // and keeps all runtime tuning centralized in one handler unit.
        WhisperMap()["status"] = [](Command const& cmd, Player* master, Player* /*bot*/) { return DoStatus(cmd, master, nullptr); };
        WhisperMap()["on"] = [](Command const& cmd, Player* master, Player* /*bot*/) { return DoOn(cmd, master, nullptr); };
        WhisperMap()["off"] = [](Command const& cmd, Player* master, Player* /*bot*/) { return DoOff(cmd, master, nullptr); };
        WhisperMap()["tick"] = [](Command const& cmd, Player* master, Player* /*bot*/) { return DoTick(cmd, master, nullptr); };
        WhisperMap()["followdist"] = [](Command const& cmd, Player* master, Player* /*bot*/) { return DoFollowDist(cmd, master, nullptr); };
        WhisperMap()["followangle"] = [](Command const& cmd, Player* master, Player* /*bot*/) { return DoFollowAngle(cmd, master, nullptr); };

        LeaderGroupMap()["reloadbots"] = &DoReloadBots;
    }
}