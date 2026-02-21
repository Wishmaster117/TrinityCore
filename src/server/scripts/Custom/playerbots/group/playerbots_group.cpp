/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"

#include "Group.h"
#include "GroupMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"

#include "playerbots/core/playerbots_bot_store.h"
#include "playerbots/core/playerbots_grouping.h"

namespace
{
    // Mirrors the essential accept-path from WorldSession::HandlePartyInviteResponseOpcode,
    // but keeps it script-side to avoid core patches.
    static bool AcceptGroupInvite(Group* group, Player* invited)
    {
        if (!group || !invited)
            return false;

        // Group is full
        if (group->IsFull())
            return false;

        // Remove player from invitees in any case
        group->RemoveInvite(invited);

        // Prevent accepting own invite (should not happen, but keep parity with core checks)
        if (group->GetLeaderGUID() == invited->GetGUID())
        {
            TC_LOG_ERROR("playerbots", "Playerbots: {} tried to accept an invite to his own group.", invited->GetName());
            return false;
        }

        // Forming a new group, create it if needed
        if (!group->IsCreated())
        {
            Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
            if (!leader)
            {
                group->RemoveAllInvites();
                return false;
            }

            // This matches the core behavior:
            // - remove leader from invites
            // - create group and register in GroupMgr
            group->RemoveInvite(leader);
            group->Create(leader);
            sGroupMgr->AddGroup(group);
        }

        // PLAYER'S GROUP IS SET IN ADDMEMBER!!!
        if (!group->AddMember(invited))
            return false;

        group->BroadcastGroupUpdate();
        return true;
    }
}

class PlayerbotsGroupScript final : public GroupScript
{
public:
    PlayerbotsGroupScript() : GroupScript("PlayerbotsGroupScript") { }

    void OnInviteMember(Group* group, ObjectGuid guid) override
    {
        // Only auto-accept for flagged bots.
        Player* invited = ObjectAccessor::FindPlayer(guid);
        if (!invited)
            return;

        Playerbots::BotStore::Instance().EnsureLoaded();
        if (!Playerbots::BotStore::Instance().IsBot(invited->GetGUID()))
            return;

        // Don't interfere if already in a group
        if (invited->GetGroup())
            return;

        // Ownership rule: if bot has an owner, only accept invites from that owner (leader of inviter group).
        ObjectGuid leaderGuid = group->GetLeaderGUID();
        uint64 leaderLow = leaderGuid.GetCounter();
        uint64 ownerLow = Playerbots::BotStore::Instance().GetOwnerLow(invited->GetGUID());
        if (ownerLow != 0 && ownerLow != leaderLow)
        {
            TC_LOG_INFO("playerbots", "Playerbots: bot {} refused invite (owner_guid={}, inviter_leader={}).",
                invited->GetName(), ownerLow, leaderLow);
            return;
        }

        // Accept the invite server-side
        if (AcceptGroupInvite(group, invited))
        {
            TC_LOG_INFO("playerbots", "Playerbots: auto-accepted group invite for bot {}.", invited->GetName());

            // Bind ownership to current group leader so chat commands work immediately.
            if (Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID()))
                Playerbots::EnsureGroupedAndOwned(invited, leader);
        }
    }

    // When a bot is removed from a group, clear owner_guid (requested behavior).
    void OnRemoveMember(Group* /*group*/, ObjectGuid guid, RemoveMethod /*method*/, ObjectGuid /*kicker*/, char const* /*reason*/) override
    {
        Playerbots::BotStore::Instance().EnsureLoaded();
        if (!Playerbots::BotStore::Instance().IsBot(guid))
            return;

        // Clear owner on uninvite/kick/leave
        Playerbots::BotStore::Instance().SetOwnerLow(guid, 0);
        TC_LOG_INFO("playerbots", "Playerbots: cleared owner for bot {}.", guid.ToString());
    }
};

void AddSC_playerbots_group()
{
    new PlayerbotsGroupScript();
}