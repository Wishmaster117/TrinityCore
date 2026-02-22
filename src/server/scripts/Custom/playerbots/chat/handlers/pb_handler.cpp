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

#include "playerbots/core/playerbots_bot_store.h"
#include "playerbots/core/playerbots_registry.h"

#include <unordered_map>
#include <functional>
#include <utility>

namespace Playerbots::PB
{
    using HandlerFn = std::function<HandlerResult(Command const&, Player*, Player*)>;
    using LeaderGroupFn = std::function<HandlerResult(Command const&, Player*, Group*)>;

    static std::unordered_map<std::string, HandlerFn> s_whisperHandlers;
    static std::unordered_map<std::string, HandlerFn> s_groupBotHandlers;
    static std::unordered_map<std::string, LeaderGroupFn> s_leaderGroupHandlers;

    static bool IsFlaggedBot(Player* player)
    {
        BotStore::Instance().EnsureLoaded();
        return BotStore::Instance().IsBot(player);
    }

    static bool IsOwnedByMaster(Player* bot, Player* master)
    {
        if (!bot || !master)
            return false;

        BotStore::Instance().EnsureLoaded();
        uint64 ownerLow = BotStore::Instance().GetOwnerLow(bot->GetGUID());
        return ownerLow != 0 && ownerLow == master->GetGUID().GetCounter();
    }

    template <typename... Args>
    static void Send(Player* master, char const* fmt, Args&&... args)
    {
        if (!master || !master->GetSession())
            return;

        ChatHandler(master->GetSession()).PSendSysMessage(fmt, std::forward<Args>(args)...);
    }

    static HandlerResult RequireFlaggedBotOwnedOrJoin(Command const& cmd, Player* master, Player* bot, bool allowJoinBootstrap)
    {
        if (!master || !bot)
            return HandlerResult::HandledError;

        if (!IsFlaggedBot(bot))
            return HandlerResult::NotHandled; // ignore non-bots silently

        if (allowJoinBootstrap && cmd.Name == "join")
            return HandlerResult::HandledOk;

        if (!IsOwnedByMaster(bot, master))
            return HandlerResult::NotHandled;

        return HandlerResult::HandledOk;
    }

    bool IsKnownCommand(std::string const& name)
    {
        if (name.empty())
            return false;

        // Single source of truth: our registered handler maps.
        return s_whisperHandlers.find(name) != s_whisperHandlers.end()
            || s_groupBotHandlers.find(name) != s_groupBotHandlers.end()
            || s_leaderGroupHandlers.find(name) != s_leaderGroupHandlers.end();
    }

    HandlerResult DispatchToWhisperedBot(Command const& cmd, Player* master, Player* bot)
    {
        // Allow 'join' bootstrap even before ownership exists.
        HandlerResult prereq = RequireFlaggedBotOwnedOrJoin(cmd, master, bot, true);
        if (prereq == HandlerResult::NotHandled)
            return HandlerResult::NotHandled;
        if (prereq == HandlerResult::HandledError)
            return HandlerResult::HandledError;

        auto it = s_whisperHandlers.find(cmd.Name);
        if (it == s_whisperHandlers.end())
            return HandlerResult::NotHandled;

        return it->second(cmd, master, bot);
    }

    HandlerResult DispatchToGroupBot(Command const& cmd, Player* master, Player* bot)
    {
        // On group broadcast, do not allow join/bootstrap; require ownership.
        HandlerResult prereq = RequireFlaggedBotOwnedOrJoin(cmd, master, bot, false);
        if (prereq != HandlerResult::HandledOk)
            return HandlerResult::NotHandled;

        auto it = s_groupBotHandlers.find(cmd.Name);
        if (it == s_groupBotHandlers.end())
            return HandlerResult::NotHandled;

        return it->second(cmd, master, bot);
    }

    HandlerResult DispatchLeaderOnlyGroupCommand(Command const& cmd, Player* master, Group* group)
    {
        auto it = s_leaderGroupHandlers.find(cmd.Name);
        if (it == s_leaderGroupHandlers.end())
            return HandlerResult::NotHandled;

        return it->second(cmd, master, group);
    }

    // Registry access for registration units.
    std::unordered_map<std::string, HandlerFn>& WhisperMap() { return s_whisperHandlers; }
    std::unordered_map<std::string, HandlerFn>& GroupBotMap() { return s_groupBotHandlers; }
    std::unordered_map<std::string, LeaderGroupFn>& LeaderGroupMap() { return s_leaderGroupHandlers; }
}