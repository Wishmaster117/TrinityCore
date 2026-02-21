/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots_spawner.h"

#include "AccountMgr.h"
#include "Map.h"
#include "ClientBuildInfo.h"
#include "DatabaseEnv.h"
#include "LoginDatabase.h"
#include "Database/Implementation/CharacterDatabase.h"
#include "Database/QueryHolder.h"
#include "fmt/format.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"

#include "playerbots/core/playerbots_bot_store.h"

#include <memory>

namespace Playerbots
{
    class PlayerbotsLoginQueryHolder final : public CharacterDatabaseQueryHolder
    {
    public:
        PlayerbotsLoginQueryHolder(uint32 accountId, ObjectGuid guid)
            : _accountId(accountId), _guid(guid) { }

        ObjectGuid GetGuid() const { return _guid; }
        uint32 GetAccountId() const { return _accountId; }

        bool Initialize();

    private:
        uint32 _accountId;
        ObjectGuid _guid;
    };

    bool PlayerbotsLoginQueryHolder::Initialize()
    {
        SetSize(MAX_PLAYER_LOGIN_QUERY);

        bool res = true;
        ObjectGuid::LowType lowGuid = _guid.GetCounter();

        // Copied from TrinityCore: src/server/game/Handlers/CharacterHandler.cpp (LoginQueryHolder::Initialize)
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_FROM, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CUSTOMIZATIONS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CUSTOMIZATIONS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GROUP_MEMBER);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GROUP, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURAS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURA_EFFECTS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURA_EFFECTS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURA_STORED_LOCATIONS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURA_STORED_LOCATIONS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELLS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL_FAVORITES);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_FAVORITES, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_OBJECTIVES, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES_CRITERIA);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_OBJECTIVES_CRITERIA, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES_CRITERIA_PROGRESS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_OBJECTIVES_CRITERIA_PROGRESS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES_SPAWN_TRACKING);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_OBJECTIVES_SPAWN_TRACKING, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_DAILY);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DAILY_QUEST_STATUS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_WEEKLY);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_WEEKLY_QUEST_STATUS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_MONTHLY);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MONTHLY_QUEST_STATUS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_SEASONAL);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SEASONAL_QUEST_STATUS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_REPUTATION);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_REPUTATION, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INVENTORY, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE_ARTIFACT);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ARTIFACTS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE_AZERITE);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AZERITE, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE_AZERITE_MILESTONE_POWER);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AZERITE_MILESTONE_POWERS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE_AZERITE_UNLOCKED_ESSENCE);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AZERITE_UNLOCKED_ESSENCES, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE_AZERITE_EMPOWERED);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AZERITE_EMPOWERED, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAIL);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAILS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS_ARTIFACT);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS_ARTIFACT, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS_AZERITE);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS_AZERITE, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS_AZERITE_MILESTONE_POWER);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS_AZERITE_MILESTONE_POWER, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS_AZERITE_UNLOCKED_ESSENCE);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS_AZERITE_UNLOCKED_ESSENCE, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS_AZERITE_EMPOWERED);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS_AZERITE_EMPOWERED, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SOCIALLIST);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SOCIAL_LIST, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_HOMEBIND);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_HOME_BIND, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELLCOOLDOWNS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_COOLDOWNS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL_CHARGES);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_CHARGES, stmt);

        if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DECLINEDNAMES);
            stmt->setUInt64(0, lowGuid);
            res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DECLINED_NAMES, stmt);
        }

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_MEMBER);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GUILD, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ARENAINFO);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ARENA_INFO, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACHIEVEMENTS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACHIEVEMENTS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CRITERIAPROGRESS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CRITERIA_PROGRESS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_EQUIPMENTSETS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_EQUIPMENT_SETS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_TRANSMOG_OUTFITS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TRANSMOG_OUTFITS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CUF_PROFILES);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CUF_PROFILES, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BGDATA);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BG_DATA, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GLYPHS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GLYPHS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_TALENTS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TALENTS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_PVP_TALENTS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_PVP_TALENTS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_ACCOUNT_DATA);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SKILLS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SKILLS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_RANDOMBG);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_RANDOM_BG, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BANNED);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BANNED, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUSREW);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_REW, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_INSTANCELOCKTIMES);
        stmt->setUInt32(0, _accountId);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INSTANCE_LOCK_TIMES, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_CURRENCY);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CURRENCY, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CORPSE_LOCATION);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CORPSE_LOCATION, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_PETS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_PET_SLOTS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GARRISON);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GARRISON, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GARRISON_BLUEPRINTS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GARRISON_BLUEPRINTS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GARRISON_BUILDINGS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GARRISON_BUILDINGS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GARRISON_FOLLOWERS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GARRISON_FOLLOWERS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GARRISON_FOLLOWER_ABILITIES);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GARRISON_FOLLOWER_ABILITIES, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_TRAIT_ENTRIES);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TRAIT_ENTRIES, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_TRAIT_CONFIGS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TRAIT_CONFIGS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_DATA_ELEMENTS_CHARACTER);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DATA_ELEMENTS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_DATA_FLAGS_CHARACTER);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DATA_FLAGS, stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BANK_TAB_SETTINGS);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BANK_TAB_SETTINGS, stmt);

        return res;
    }
	   
    static bool IsOffline(ObjectGuid botGuid)
    {
        QueryResult r = CharacterDatabase.Query(fmt::format(
            "SELECT online FROM characters WHERE guid={} LIMIT 1", uint32(botGuid.GetCounter())).c_str());
        if (!r)
            return false;
        return r->Fetch()[0].GetUInt8() == 0;
    }

    static ObjectGuid FindFreeRandomBotGuid()
    {
        QueryResult r = CharacterDatabase.Query(
            "SELECT b.guid "
            "FROM playerbots_bots b "
            "JOIN characters c ON c.guid=b.guid "
            "WHERE b.bot_type=1 AND b.owner_guid=0 AND c.online=0 "
            "ORDER BY b.guid ASC LIMIT 1");
        if (!r)
            return ObjectGuid::Empty;

        uint64 guidLow = r->Fetch()[0].GetUInt64();
        return ObjectGuid::Create<HighGuid::Player>(guidLow);
    }

    static uint32 GetBotAccountId(ObjectGuid botGuid)
    {
        QueryResult r = CharacterDatabase.Query(fmt::format(
            "SELECT account FROM characters WHERE guid={} LIMIT 1", uint32(botGuid.GetCounter())).c_str());
        if (!r)
            return 0;
        return r->Fetch()[0].GetUInt32();
    }

    static void SetCharacterOnlineFlag(ObjectGuid botGuid, bool online)
    {
        CharacterDatabase.DirectExecute(fmt::format(
            "UPDATE characters SET online={} WHERE guid={}", online ? 1 : 0, uint32(botGuid.GetCounter())).c_str());
    }

    static uint32 GetBattlenetAccountIdForGameAccount(uint32 accountId)
    {
        // Prefer auth.account.battlenet_account when present/filled.
        if (QueryResult r = LoginDatabase.Query(
            fmt::format("SELECT battlenet_account FROM account WHERE id={} LIMIT 1", accountId).c_str()))
        {
            uint32 bnet = r->Fetch()[0].GetUInt32();
            if (bnet)
                return bnet;
        }

        // Fallback: pick any existing battlenet_accounts.id (avoids FK crash on bnet tables).
        if (QueryResult r2 = LoginDatabase.Query("SELECT id FROM battlenet_accounts ORDER BY id ASC LIMIT 1"))
            return r2->Fetch()[0].GetUInt32();

        return 0;
    }

    Player* Spawner::SpawnByGuid(Player* requester, ObjectGuid botGuid, bool reserveForRequester)
    {
        return SpawnInternal(requester, botGuid, reserveForRequester);
    }

    Player* Spawner::SpawnRandom(Player* requester, bool reserveForRequester)
    {
        return SpawnInternal(requester, FindFreeRandomBotGuid(), reserveForRequester);
    }

    bool Spawner::DespawnBot(Player* bot, bool saveToDB)
    {
        if (!bot)
            return false;

        ObjectGuid botGuid = bot->GetGUID();
        WorldSession* session = bot->GetSession();

        // Best-effort: mark offline first to avoid re-entrancy/race with other systems.
        SetCharacterOnlineFlag(botGuid, false);

        // Optionally save (avoid heavy DB churn if you despawn hundreds at shutdown).
        if (saveToDB)
        {
            // TrinityCore requires both LoginDatabase + CharacterDatabase transactions.
            CharacterDatabaseTransaction characterTrans = CharacterDatabase.BeginTransaction();
            LoginDatabaseTransaction loginTrans = LoginDatabase.BeginTransaction();

            bot->SaveToDB(loginTrans, characterTrans, true);

            CharacterDatabase.CommitTransaction(characterTrans);
            LoginDatabase.CommitTransaction(loginTrans);
        }

        // Remove from map/world without the normal session pipeline.
        if (bot->IsInWorld())
        {
            if (Map* map = bot->GetMap())
                map->RemovePlayerFromMap(bot, false);
        }

        // Remove from global accessor so ObjectAccessor::FindPlayer no longer returns it.
        ObjectAccessor::RemoveObject(bot);

        // Detach player from session to prevent double-free paths.
        if (session)
            session->SetPlayer(nullptr);

        // Make sure internal subsystems are cleaned (pets, auras, transports, etc.).
        bot->CleanupsBeforeDelete();
        delete bot;

        // Headless sessions are NOT registered in World session list, so we must delete manually.
        delete session;

        return true;
    }

    Player* Spawner::SpawnInternal(Player* requester, ObjectGuid botGuid, bool reserveForRequester)
    {
        if (!botGuid)
            return nullptr;

        // Reservation requires a requester (GM/owner). Auto-spawn uses reserveForRequester=false.
        if (reserveForRequester && !requester)
            return nullptr;

        // Anti-double-spawn:
        // If the bot is already in memory (online), do NOT create a second Player* with the same guid.
        if (Player* existing = ObjectAccessor::FindPlayer(botGuid))
        {
            BotStore::Instance().EnsureLoaded();

            if (reserveForRequester)
            {
                uint64 reqLow = requester->GetGUID().GetCounter();
                uint64 ownerLow = BotStore::Instance().GetOwnerLow(botGuid);

                // If reserved by another player, deny.
                if (ownerLow && ownerLow != reqLow)
                    return nullptr;

                // If not reserved yet, reserve now.
                if (!ownerLow)
                    BotStore::Instance().SetOwnerLow(botGuid, reqLow);
            }

            return existing;
        }

        BotStore::Instance().EnsureLoaded();
        if (!BotStore::Instance().IsBot(botGuid))
            return nullptr;

        if (!IsOffline(botGuid))
            return nullptr;

        if (reserveForRequester)
            BotStore::Instance().SetOwnerLow(botGuid, requester->GetGUID().GetCounter());

        uint32 accountId = GetBotAccountId(botGuid);
        if (!accountId)
            return nullptr;

        std::string accountName;
        if (!AccountMgr::GetName(accountId, accountName))
            accountName = "playerbot";

        uint32 battlenetAccountId = GetBattlenetAccountIdForGameAccount(accountId);
        if (!battlenetAccountId)
        {
            TC_LOG_ERROR("playerbots", "Playerbots: cannot spawn bot {}. No battlenet account id found (auth.battlenet_accounts empty or account not linked).", botGuid.ToString());
            return nullptr;
        }

        auto nullSock = std::shared_ptr<WorldSocket>();

        // IMPORTANT: WorldSession ctor order matters (bnet id must be param #3).
        WorldSession* session = new WorldSession(
            accountId,
            std::string(accountName),
            battlenetAccountId,
            std::string(),                 // battlenet email (unused for headless)
            std::move(nullSock),
            SEC_PLAYER,
            sWorld->getIntConfig(CONFIG_EXPANSION),
            0,                             // mute_time
            std::string("playerbots"),      // os tag
            Minutes(0),                    // timezoneOffset
            0,                             // build
            ClientBuild::VariantId{},      // clientBuildVariant
            LOCALE_enUS,                   // locale
            0,                             // recruiter
            false);

        // Headless bot session: disable pointless packet sending/log spam in core.
        session->SetHeadlessBotSession(true);

        // Headless: no socket. Prevent idle kick path in WorldSession::Update().
        session->SetInQueue(true);

        // Headless session: DO NOT register it in World session list.
        // Registering a session without socket triggers idle/logout logic and packet spam,
        // and can also hit bnet-account persistence paths (FK errors) and/or crashes.

        std::shared_ptr<PlayerbotsLoginQueryHolder> holder = std::make_shared<PlayerbotsLoginQueryHolder>(accountId, botGuid);
        if (!holder->Initialize())
            return nullptr;

        SQLQueryHolderCallback callback = CharacterDatabase.DelayQueryHolder(holder);
        callback.m_future.wait();

        Player* bot = new Player(session);
        if (!bot->LoadFromDB(botGuid, *holder))
        {
            session->SetPlayer(nullptr);
            delete bot;
            delete session;
            return nullptr;
        }

        session->SetPlayer(bot);
        bot->GetMotionMaster()->Initialize();

        if (!bot->GetMap()->AddPlayerToMap(bot))
        {
            session->SetPlayer(nullptr);
            delete bot;
            delete session;
            return nullptr;
        }

        ObjectAccessor::AddObject(bot);
        SetCharacterOnlineFlag(botGuid, true);

        if (requester)
            TC_LOG_INFO("server.loading", "Playerbots: spawned bot {} ({}) by GM {}.",
                bot->GetName(), botGuid.ToString(), requester->GetName());
        else
            TC_LOG_INFO("server.loading", "Playerbots: auto-spawned bot {} ({}).", bot->GetName(), botGuid.ToString());
         return bot;
    }
}