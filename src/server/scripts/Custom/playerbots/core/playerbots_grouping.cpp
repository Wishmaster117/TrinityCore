/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots_grouping.h"

#include "Group.h"
#include "Player.h"

#include "playerbots/core/playerbots_bot_store.h"
#include "playerbots/core/playerbots_registry.h"

namespace Playerbots
{
    bool EnsureGroupedAndOwned(Player* bot, Player* master)
    {
        if (!bot || !master)
            return false;

        BotStore::Instance().EnsureLoaded();
        if (!BotStore::Instance().IsBot(bot))
            return false;

        // Ownership rule:
        // - if bot has an owner, only that owner can (re)group it
        uint64 botOwnerLow = BotStore::Instance().GetOwnerLow(bot->GetGUID());
        uint64 masterLow = master->GetGUID().GetCounter();
        if (botOwnerLow != 0 && botOwnerLow != masterLow)
            return false;

        Group* group = master->GetGroup();
        if (!group)
        {
            group = new Group();
            if (!group->Create(master))
            {
                delete group;
                return false;
            }
        }

        // Bot already in some group?
        if (Group* botGroup = bot->GetGroup())
        {
            if (botGroup != group)
                return false; // refuse cross-group join
        }
        else
        {
            if (!group->AddMember(bot))
                return false;
        }

        // Register ownership
        Registry::Instance().Add(bot, master);
        Registry::Instance().SetPaused(bot, false);
        Registry::Instance().SetFollowing(bot, false);

        // Persist owner for later (and to refuse other invites)
        BotStore::Instance().SetOwnerLow(bot->GetGUID(), masterLow);
        return true;
    }
}