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
#include "Group.h"
#include "Player.h"
#include "WorldSession.h"

#include "playerbots/core/playerbots_manager.h"
#include "playerbots/formation/playerbots_formation.h"

namespace Playerbots::PB
{
    std::unordered_map<std::string, std::function<HandlerResult(Command const&, Player*, Group*)>>& LeaderGroupMap();
    std::unordered_map<std::string, std::function<HandlerResult(Command const&, Player*, Player*)>>& WhisperMap();

    static bool TryParseFormation(Command const& cmd, Playerbots::Formation::Type& out)
    {
        if (cmd.Args.size() < 2)
            return false;

        std::string const& form = cmd.Args[1];
        if (form == "arrow")      out = Playerbots::Formation::Type::Arrow;
        else if (form == "queue") out = Playerbots::Formation::Type::Queue;
        else if (form == "near")  out = Playerbots::Formation::Type::Near;
        else if (form == "melee") out = Playerbots::Formation::Type::Melee;
        else if (form == "line")  out = Playerbots::Formation::Type::Line;
        else if (form == "circle")out = Playerbots::Formation::Type::Circle;
        else if (form == "chaos") out = Playerbots::Formation::Type::Chaos;
        else if (form == "shield")out = Playerbots::Formation::Type::Shield;
        else
            return false;

        return true;
    }

    static HandlerResult DoFormationLeader(Command const& cmd, Player* master, Group* group)
    {
        if (!master || !group)
            return HandlerResult::HandledError;

        // Formation is a leader-level command (allow assistant for convenience).
        ObjectGuid masterGuid = master->GetGUID();
        if (!(group->IsLeader(masterGuid) || group->IsAssistant(masterGuid)))
            return HandlerResult::HandledError;

        Playerbots::Formation::Type type;
        if (!TryParseFormation(cmd, type))
        {
            ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: usage: formation <arrow|queue|near|melee|line|circle|chaos|shield>.");
            return HandlerResult::HandledError;
        }

        Manager::Instance().SetFormationFor(master->GetGUID(), type);
        ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: formation set to '%s'.", cmd.Args[1].c_str());
        return HandlerResult::HandledOk;
    }

    static HandlerResult DoFormationWhisper(Command const& cmd, Player* master, Player* /*bot*/)
    {
        // Whisper path also sets leader-level formation (handy when whispering any owned bot).
        Group* group = master ? master->GetGroup() : nullptr;
        if (!group)
        {
            ChatHandler(master->GetSession()).PSendSysMessage("Playerbots: you must be in a group to set formation.");
            return HandlerResult::HandledError;
        }

        return DoFormationLeader(cmd, master, group);
    }

    void RegisterFormationHandlers()
    {
        LeaderGroupMap()["formation"] = &DoFormationLeader;
        WhisperMap()["formation"] = &DoFormationWhisper;
    }
}