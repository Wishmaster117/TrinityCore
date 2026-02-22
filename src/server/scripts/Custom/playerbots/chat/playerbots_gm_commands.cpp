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
#include "ChatCommandTags.h"
#include "ChatCommand.h"
#include "DatabaseEnv.h"
#include "fmt/format.h"
#include "Player.h"
#include "ObjectAccessor.h"

#include "playerbots/core/playerbots_spawner.h"

using namespace Trinity::ChatCommands;

namespace Playerbots
{
    static ObjectGuid ResolveBotGuid(Variant<uint32, QuotedString> target)
    {
        ObjectGuid botGuid = ObjectGuid::Empty;

        if (std::holds_alternative<uint32>(target))
            botGuid = ObjectGuid::Create<HighGuid::Player>(std::get<uint32>(target));
        else
        {
            std::string name = std::get<QuotedString>(target);
            CharacterDatabase.EscapeString(name);

            QueryResult r = CharacterDatabase.Query(fmt::format(
                "SELECT guid FROM characters WHERE name='{}' LIMIT 1", name).c_str());
            if (r)
                botGuid = ObjectGuid::Create<HighGuid::Player>(r->Fetch()[0].GetUInt32());
        }

        return botGuid;
    }

    class PlayerbotsCommandScript final : public CommandScript
    {
    public:
        PlayerbotsCommandScript() : CommandScript("PlayerbotsCommandScript") { }

        ChatCommandTable GetCommands() const override
        {
            static ChatCommandTable pbCommandTable =
            {
                { "spawn",       HandleSpawn,       rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
                { "spawnrandom", HandleSpawnRandom, rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
                { "goto",        HandleGoto,        rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
                { "where",       HandleWhere,       rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
            };

            static ChatCommandTable commandTable =
            {
                { "pb", pbCommandTable },
            };

            return commandTable;
        }

        static bool HandleSpawn(ChatHandler* handler, Variant<uint32, QuotedString> target)
        {
            Player* requester = handler ? handler->GetPlayer() : nullptr;
            if (!requester)
                return false;

            ObjectGuid botGuid = ResolveBotGuid(target);

            if (!botGuid)
            {
                handler->SendSysMessage("Playerbots: spawn failed (invalid bot name/guid).", false);
                return false;
            }

            Player* bot = Spawner::SpawnByGuid(requester, botGuid, /*reserveForRequester=*/true);
            if (!bot)
            {
                handler->PSendSysMessage("Playerbots: spawn failed for %s.", botGuid.ToString().c_str());
                return false;
            }

            handler->PSendSysMessage("Playerbots: spawned bot %s (guid=%s, low=%u).",
                bot->GetName().c_str(), botGuid.ToString().c_str(), uint32(botGuid.GetCounter()));
            handler->PSendSysMessage("Playerbots: /w %s join  then  /w %s follow.", bot->GetName().c_str(), bot->GetName().c_str());
            handler->PSendSysMessage("Playerbots: use .pb goto %s to teleport to the bot (GM summon won't work for headless bots).", bot->GetName().c_str());
            return true;
        }

        static bool HandleSpawnRandom(ChatHandler* handler)
        {
            Player* requester = handler ? handler->GetPlayer() : nullptr;
            if (!requester)
                return false;

            Player* bot = Spawner::SpawnRandom(requester, /*reserveForRequester=*/true);
            if (!bot)
            {
                handler->SendSysMessage("Playerbots: spawnrandom failed (no free offline bot, or missing battlenet account).", false);
                return false;
            }

            // NOTE: PSendSysMessage is printf-style, NOT fmt-style.
            handler->PSendSysMessage("Playerbots: spawned bot %s (%s).", bot->GetName().c_str(), bot->GetGUID().ToString().c_str());
            handler->PSendSysMessage("Playerbots: /w %s join then /w %s follow.", bot->GetName().c_str(), bot->GetName().c_str());
            handler->PSendSysMessage("Playerbots: use .pb goto %s to teleport to the bot (GM summon /who won't reflect headless bots).", bot->GetName().c_str());
            return true;
        }

        static bool HandleWhere(ChatHandler* handler, Variant<uint32, QuotedString> target)
        {
            Player* requester = handler ? handler->GetPlayer() : nullptr;
            if (!requester)
                return false;

            ObjectGuid botGuid = ResolveBotGuid(target);
            if (!botGuid)
            {
                handler->SendSysMessage("Playerbots: where failed (invalid bot name/guid).", false);
                return false;
            }

            Player* bot = ObjectAccessor::FindPlayer(botGuid);
            if (!bot)
            {
                // Headless bots are normally offline. For convenience, spawn on-demand.
                bot = Spawner::SpawnByGuid(requester, botGuid, /*reserveForRequester=*/true);
                if (!bot)
                {
                    handler->PSendSysMessage("Playerbots: bot %s is not online in world and spawn failed.", botGuid.ToString().c_str());
                    return false;
                }
            }

            handler->PSendSysMessage("Playerbots: %s (%s) map=%u X=%.3f Y=%.3f Z=%.3f O=%.3f",
                bot->GetName().c_str(), botGuid.ToString().c_str(), bot->GetMapId(),
                bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetOrientation());
            return true;
        }

        static bool HandleGoto(ChatHandler* handler, Variant<uint32, QuotedString> target)
        {
            Player* requester = handler ? handler->GetPlayer() : nullptr;
            if (!requester)
                return false;

            ObjectGuid botGuid = ResolveBotGuid(target);
            if (!botGuid)
            {
                handler->SendSysMessage("Playerbots: goto failed (invalid bot name/guid).", false);
                return false;
            }

            Player* bot = ObjectAccessor::FindPlayer(botGuid);
            if (!bot)
            {
                handler->PSendSysMessage("Playerbots: bot %s is not online in world.", botGuid.ToString().c_str());
                return false;
            }

            requester->TeleportTo(bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetOrientation());
            handler->PSendSysMessage("Playerbots: teleported to %s (%s).", bot->GetName().c_str(), botGuid.ToString().c_str());
            return true;
        }
    };
}

void AddSC_playerbots_commands()
{
    new Playerbots::PlayerbotsCommandScript();
}