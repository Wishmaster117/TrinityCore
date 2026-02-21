/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots/chat/command_dispatcher.h"

#include "Chat.h"
#include "ChatPackets.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"

#include "playerbots/chat/handlers/pb_handler.h"
#include "playerbots/core/playerbots_bot_store.h"
#include "playerbots/core/playerbots_manager.h"

#include <chrono>
#include <unordered_map>
#include <memory>

namespace Playerbots
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        // Rate limiting is per (master,bot) and per (master,0) to prevent spam across many bots.
        struct RateKey
        {
            uint64 MasterLow = 0;
            uint64 BotLow = 0;

            bool operator==(RateKey const& r) const { return MasterLow == r.MasterLow && BotLow == r.BotLow; }
        };

        struct RateKeyHash
        {
            size_t operator()(RateKey const& k) const noexcept
            {
                // Simple combine; good enough for small maps.
                return std::hash<uint64>()(k.MasterLow) ^ (std::hash<uint64>()(k.BotLow) << 1);
            }
        };

        static std::unordered_map<RateKey, Clock::time_point, RateKeyHash> s_lastCmd;
        static std::unordered_map<uint64, Clock::time_point> s_lastWarn; // masterLow -> last "slow down" feedback

        static void SendFeedbackWhisper(Player* bot, Player* master, std::string const& msg)
        {
            if (!master)
                return;

            // Best effort: whisper from bot to master when possible, otherwise fallback to sysmsg.
            if (bot)
            {
                // TrinityCore Player::Whisper exists on all modern branches.
                bot->Whisper(msg, LANG_UNIVERSAL, master);
                return;
            }

            if (master->GetSession())
                ChatHandler(master->GetSession()).SendSysMessage(msg.c_str());
        }

        static ChatMsg ToChatMsg(ChatChannel ch)
        {
            switch (ch)
            {
                case ChatChannel::Party:       return CHAT_MSG_PARTY;
                case ChatChannel::Raid:        return CHAT_MSG_RAID;
                case ChatChannel::RaidWarning: return CHAT_MSG_RAID_WARNING;
                case ChatChannel::Instance:    return CHAT_MSG_INSTANCE_CHAT;
                default:                       return CHAT_MSG_PARTY;
            }
        }

        static void SendFeedbackGroup(Player* master, Group* group, ChatChannel channel, std::string const& msg)
        {
            if (!master || !group || msg.empty())
                return;

            ChatMsg chatType = ToChatMsg(channel);

            // PARTY messages should go only to subgroup.
            int subgroup = -1;
            if (chatType == CHAT_MSG_PARTY)
                subgroup = master->GetSubGroup();

            // Broadcast as the master (clean and always valid). Language in group channels is typically universal anyway.
            WorldPackets::Chat::Chat packet;
            packet.Initialize(chatType, LANG_UNIVERSAL, master, nullptr, msg, 0, "", DEFAULT_LOCALE);
            group->BroadcastPacket(packet.Write(), /*ignorePlayersInBGRaid=*/true, subgroup, ObjectGuid::Empty);
        }

        static void SendFeedback(Player* bot, Player* master, Group* group, ChatChannel channel, std::string const& msg)
        {
            if (channel == ChatChannel::Whisper)
            {
                SendFeedbackWhisper(bot, master, msg);
                return;
            }

            SendFeedbackGroup(master, group, channel, msg);
        }

        static bool IsRateLimited(Player* master, Player* bot)
        {
            if (!master || !bot)
                return false;

            RateKey key;
            key.MasterLow = master->GetGUID().GetCounter();
            key.BotLow = bot->GetGUID().GetCounter();

            auto now = Clock::now();
            uint32 minMs = Manager::Instance().GetCommandMinIntervalMs();
            auto minInterval = std::chrono::milliseconds(minMs);

            // 1) Per-master limiter (BotLow=0): prevents spamming many bots quickly.
            {
                RateKey mkey;
                mkey.MasterLow = key.MasterLow;
                mkey.BotLow = 0;

                auto itM = s_lastCmd.find(mkey);
                if (itM != s_lastCmd.end())
                {
                    if (minMs > 0 && (now - itM->second < minInterval))
                        return true;
                    itM->second = now;
                }
                else
                    s_lastCmd.emplace(mkey, now);
            }

            // 2) Per-(master,bot) limiter
            auto it = s_lastCmd.find(key);
            if (it != s_lastCmd.end())
            {
                if (minMs > 0 && (now - it->second < minInterval))
                    return true;
                it->second = now;
                return false;
            }

            s_lastCmd.emplace(key, now);
            return false;
        }

        static bool ShouldWarnRateLimited(Player* master)
        {
            if (!master)
                return false;

            uint64 masterLow = master->GetGUID().GetCounter();
            auto now = Clock::now();

            auto it = s_lastWarn.find(masterLow);
            if (it != s_lastWarn.end())
            {
                if (now - it->second < std::chrono::seconds(1))
                    return false;
                it->second = now;
                return true;
            }

            s_lastWarn.emplace(masterLow, now);
            return true;
        }

        static bool CheckOwnership(Player* master, Player* bot, Command const& cmd, ChatChannel channel)
        {
            if (!master || !bot)
                return false;

            BotStore::Instance().EnsureLoaded();
            if (!BotStore::Instance().IsBot(bot))
                return false; // not a bot, ignore

            uint64 masterLow = master->GetGUID().GetCounter();
            uint64 ownerLow  = BotStore::Instance().GetOwnerLow(bot->GetGUID());

            // Whisper bootstrap: allow "join" when unowned.
            if (channel == ChatChannel::Whisper && cmd.Name == "join" && ownerLow == 0)
                return true;

            return ownerLow != 0 && ownerLow == masterLow;;
        }
    }

    CommandDispatcher& CommandDispatcher::Instance()
    {
        static CommandDispatcher instance;
        return instance;
    }

    CommandDispatcher::CommandDispatcher()
    {
        PB::RegisterAllHandlers();
    }

    void CommandDispatcher::Dispatch(Command const& cmd, CommandContext& ctx)
    {
        if (ctx.Channel == ChatChannel::Whisper)
            (void)DispatchWhisper(cmd, ctx);
        else
            DispatchBroadcast(cmd, ctx);
    }

    bool CommandDispatcher::DispatchWhisper(Command const& cmd, CommandContext& ctx)
    {
        if (!ctx.Master || !ctx.Bot)
            return false;

        // Ownership security (DB owner_guid).
        if (!CheckOwnership(ctx.Master, ctx.Bot, cmd, ChatChannel::Whisper))
        {
            // If the target is not a bot, keep silent (avoid noise).
            BotStore::Instance().EnsureLoaded();
            if (BotStore::Instance().IsBot(ctx.Bot))
                SendFeedback(ctx.Bot, ctx.Master, ctx.Group, ctx.Channel, "Playerbots: this bot is not yours (or not reserved). Use 'join' first.");
            return false;
        }

        // Prevent spam first (even if command is invalid).
        if (IsRateLimited(ctx.Master, ctx.Bot))
        {
            if (ShouldWarnRateLimited(ctx.Master))
                SendFeedback(ctx.Bot, ctx.Master, ctx.Group, ctx.Channel, "Playerbots: slow down (rate limited).");
            return false;
        }

        PB::HandlerResult r = PB::DispatchToWhisperedBot(cmd, ctx.Master, ctx.Bot);
        if (r == PB::HandlerResult::HandledOk)
            return true;

        if (r == PB::HandlerResult::HandledError)
            return true;

        // Unknown command: keep feedback only on whispers (avoid group spam).
        SendFeedback(ctx.Bot, ctx.Master, ctx.Group, ctx.Channel, "Playerbots: unknown command '" + cmd.Name + "' (try: help).");
        return false;
    }

    void CommandDispatcher::DispatchBroadcast(Command const& cmd, CommandContext& ctx)
    {
        if (!ctx.Master || !ctx.Group)
            return;

        ObjectGuid masterGuid = ctx.Master->GetGUID();

        bool canBroadcast = ctx.Group->IsLeader(masterGuid) || ctx.Group->IsAssistant(masterGuid);
        if (!canBroadcast)
            return;

        // Some commands are "leader level" and should not iterate over bots.
        if (PB::DispatchLeaderOnlyGroupCommand(cmd, ctx.Master, ctx.Group) != PB::HandlerResult::NotHandled)
            return;

        uint32 handledCount = 0;

        auto const& slots = ctx.Group->GetMemberSlots();
        for (auto const& slot : slots)
        {
            if (!slot.guid)
                continue;

            Player* member = ObjectAccessor::FindPlayer(slot.guid);
            if (!member || member == ctx.Master)
                continue;

            // Ownership security (DB owner_guid). No bootstrap in group broadcasts.
            if (!CheckOwnership(ctx.Master, member, cmd, ctx.Channel))
                continue;

            // Rate limiting per bot on group path too.
            if (IsRateLimited(ctx.Master, member))
                continue;

            PB::HandlerResult r = PB::DispatchToGroupBot(cmd, ctx.Master, member);
            if (r == PB::HandlerResult::HandledOk)
                ++handledCount;
        }

        if (handledCount > 0)
        {
            SendFeedback(nullptr, ctx.Master, ctx.Group, ctx.Channel,
                "Playerbots: broadcast '" + cmd.Name + "' to " + std::to_string(handledCount) + " bot(s).");
        }
    }
}