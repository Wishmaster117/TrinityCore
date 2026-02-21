/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots/randombots/playerbots_rndbots_bootstrap.h"

#include "AccountMgr.h"
#include "CharacterCache.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Random.h"
#include "MapManager.h"
#include "Optional.h"
#include "fmt/format.h"
#include "ObjectMgr.h"
#include "CharacterCache.h"
#include "CharacterPackets.h"
#include "DB2Stores.h"
#include "Duration.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Realm/ClientBuildInfo.h"
#include "SharedDefines.h"
#include "World.h"
#include "WorldSession.h"
#include "WorldSocket.h"

#include <algorithm>
#include <unordered_map>
#include <cctype>
#include <random>
#include <string>
#include <vector>

namespace Playerbots::RandomBots
{
    namespace
    {
        struct SpawnHub
        {
            uint16 MapId = 0;
            float X = 0.0f;
            float Y = 0.0f;
            float Z = 0.0f;
            float O = 0.0f;
            uint32 AreaId = 0;
        };

        struct BotAccount
        {
            uint32 Id = 0;
            uint32 Index = 0;
            std::string Username;
        };

        enum : uint8
        {
            BOT_TYPE_RANDOM = 1, // keep consistent with your module enum if you add one later
        };

        // TrinityCore default realm character slots per account.
        constexpr uint32 kMaxCharsPerAccount = 10;
        constexpr uint8  kAccountTypeRandom  = 1;

        static bool TableExistsInCharacters(char const* tableName)
        {
            // Cached exact check (more reliable than SHOW TABLES LIKE, avoids LIKE wildcards and collation quirks).
            static std::unordered_map<std::string, bool> cache;

            auto it = cache.find(tableName);
            if (it != cache.end())
                return it->second;

            // DATABASE() returns the current schema for this connection (characters DB here).
            // Using information_schema.tables with an exact match on table_name is reliable.
            QueryResult r = CharacterDatabase.Query(fmt::format(
                "SELECT 1 "
                "FROM information_schema.tables "
                "WHERE table_schema = DATABASE() AND table_name = '{}' "
                "LIMIT 1",
                tableName).c_str());

            bool exists = (r != nullptr);
            cache.emplace(tableName, exists);
            return exists;
        }

        static uint32 GetIntCompat(char const* keyNew, char const* keyOld, uint32 def)
        {
            uint32 v = sConfigMgr->GetIntDefault(keyNew, def);
            if (v == def)
                v = sConfigMgr->GetIntDefault(keyOld, def);
            return v;
        }

        static bool GetBoolCompat(char const* keyNew, char const* keyOld, bool def)
        {
            bool v = sConfigMgr->GetBoolDefault(keyNew, def);
            if (v == def)
                v = sConfigMgr->GetBoolDefault(keyOld, def);
            return v;
        }

        static uint8 PickBotLevel()
        {
            uint32 levelMin = sConfigMgr->GetIntDefault("Playerbots.RandomBots.LevelMin", 1);
            uint32 levelMax = sConfigMgr->GetIntDefault("Playerbots.RandomBots.LevelMax", 1);

            if (levelMin < 1)
                levelMin = 1;
            if (levelMax < 1)
                levelMax = 1;
            if (levelMax < levelMin)
                std::swap(levelMin, levelMax);

            uint32 gameMax = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
            levelMin = std::min(levelMin, gameMax);
            levelMax = std::min(levelMax, gameMax);

            if (levelMin == levelMax)
                return uint8(levelMin);

            uint32 chanceMin = sConfigMgr->GetIntDefault("Playerbots.RandomBots.LevelChanceMinPct", 0);
            uint32 chanceMax = sConfigMgr->GetIntDefault("Playerbots.RandomBots.LevelChanceMaxPct", 0);

            if (chanceMin > 100)
                chanceMin = 100;
            if (chanceMax > 100)
                chanceMax = 100;

            // Ensure sane distribution.
            if (chanceMin + chanceMax > 100)
            {
                uint32 total = chanceMin + chanceMax;
                chanceMin = (chanceMin * 100) / total;
                chanceMax = 100 - chanceMin;
            }

            uint32 roll = urand(0, 99);
            if (roll < chanceMin)
                return uint8(levelMin);

            if (roll < chanceMin + chanceMax)
                return uint8(levelMax);

            return uint8(urand(levelMin, levelMax));
        }

        static uint8 PickCreateModeForLevel1(uint8 race, uint8 cls)
        {
            // TrinityCore: NPE is driven by PlayerInfo::createPositionNPE.
            PlayerInfo const* info = sObjectMgr->GetPlayerInfo(race, cls);
            if (!info || !info->createPositionNPE)
                return uint8(PlayerCreateMode::Normal);

            uint32 chance = sConfigMgr->GetIntDefault("Playerbots.RandomBots.Level1.UseNPEChancePct", 50);
            if (chance > 100)
                chance = 100;

            return (urand(0, 99) < chance) ? uint8(PlayerCreateMode::NPE) : uint8(PlayerCreateMode::Normal);
        }

        static Optional<SpawnHub> PickSpawnHubFor(uint8 teamId, uint8 level)
        {
            if (!sConfigMgr->GetBoolDefault("Playerbots.RandomBots.UseSpawnHubs", false))
                return {};

            if (!TableExistsInCharacters("playerbots_spawn_hubs"))
                return {};

            QueryResult result = CharacterDatabase.Query(fmt::format(
                "SELECT map, x, y, z, o, areaId FROM playerbots_spawn_hubs "
                "WHERE teamId={} AND level_min<={} AND level_max>={} ORDER BY RAND() LIMIT 1",
                uint32(teamId), uint32(level), uint32(level)).c_str());

            if (!result)
                return {};

            Field* f = result->Fetch();
            SpawnHub hub;
            hub.MapId = f[0].GetUInt16();
            hub.X = f[1].GetFloat();
            hub.Y = f[2].GetFloat();
            hub.Z = f[3].GetFloat();
            hub.O = f[4].GetFloat();
            hub.AreaId = f[5].GetUInt32();
            return hub;
        }

        static uint32 CountNamePoolFree()
        {
            // Pool is static (do not consume names). We just expose its size.
            QueryResult r = CharacterDatabase.Query("SELECT COUNT(*) FROM playerbots_names");
            if (!r)
                return 0;

            return r->Fetch()[0].GetUInt32();
        }

        static bool CharacterNameExists(std::string const& name)
        {
            std::string escaped = name;
            CharacterDatabase.EscapeString(escaped);

            // Use DB collation semantics (matches UNIQUE index behavior).
            QueryResult r = CharacterDatabase.Query(fmt::format(
                "SELECT 1 FROM characters WHERE name='{}' LIMIT 1",
                escaped).c_str());
            return r != nullptr;
        }

        static bool CharacterGuidExists(ObjectGuid::LowType guidLow)
        {
            QueryResult r = CharacterDatabase.Query(fmt::format(
                "SELECT 1 FROM characters WHERE guid={} LIMIT 1",
                uint32(guidLow)).c_str());
            return r != nullptr;
        }

        // Pick a free name from the pool (pool is static; we do NOT delete from playerbots_names).
        // IMPORTANT: must be robust against case-insensitive collations and prior runs.
        static std::string PickFreeNameFromPool()
        {
            // Select a name that does not currently exist in characters.
            // IMPORTANT: compare using the DB collation (same semantics as characters.idx_name).
            QueryResult r = CharacterDatabase.Query(
                "SELECT n.name "
                "FROM playerbots_names n "
                "WHERE NOT EXISTS ("
                "  SELECT 1 FROM characters c WHERE c.name = n.name"
                ") "
                "ORDER BY RAND() LIMIT 1");

            if (!r)
                return {};

            return r->Fetch()[0].GetString();
        }

        static void CleanupCharacterRowsForGuid(ObjectGuid::LowType guidLow, CharacterDatabaseTransaction trans)
        {
            // If only some tables were truncated, the GUID generator (based on characters.guid) can reuse a guid that
            // still exists in other character_* tables. This causes duplicate key errors during SaveToDB.
            // Clean the minimal set of tables that are known to be written during create/save.
            uint32 guid = uint32(guidLow);

            if (TableExistsInCharacters("character_action"))
                trans->Append(fmt::format("DELETE FROM character_action WHERE guid={}", guid).c_str());

            if (TableExistsInCharacters("character_skills"))
                trans->Append(fmt::format("DELETE FROM character_skills WHERE guid={}", guid).c_str());

            if (TableExistsInCharacters("character_homebind"))
                trans->Append(fmt::format("DELETE FROM character_homebind WHERE guid={}", guid).c_str());
        }

        static ObjectGuid::LowType GenerateFreeCharacterGuidLow()
        {
            // Try to find a GUID that is not present in the tables we care about.
            // This is cheap enough for bootstrap and prevents endless duplicate key spam.
            for (uint32 tries = 0; tries < 1000; ++tries)
            {
                ObjectGuid::LowType guidLow = sObjectMgr->GetGenerator<HighGuid::Player>().Generate();

                if (CharacterGuidExists(guidLow))
                    continue;

                uint32 guid = uint32(guidLow);

                if (TableExistsInCharacters("character_skills"))
                {
                    QueryResult r = CharacterDatabase.Query(fmt::format("SELECT 1 FROM character_skills WHERE guid={} LIMIT 1", guid).c_str());
                    if (r)
                        continue;
                }

                if (TableExistsInCharacters("character_action"))
                {
                    QueryResult r = CharacterDatabase.Query(fmt::format("SELECT 1 FROM character_action WHERE guid={} LIMIT 1", guid).c_str());
                    if (r)
                        continue;
                }

                return guidLow;
            }

            return sObjectMgr->GetGenerator<HighGuid::Player>().Generate();
        }

        static uint32 CountCharactersForAccount(uint32 accountId)
        {
            QueryResult r = CharacterDatabase.Query(fmt::format("SELECT COUNT(*) FROM characters WHERE account={}", accountId).c_str());
            if (!r)
                return 0;
            return r->Fetch()[0].GetUInt32();
        }

        static uint32 CountRandomPoolAccounts()
        {
            QueryResult r = CharacterDatabase.Query("SELECT COUNT(*) FROM playerbots_accounts WHERE account_type=1");
            if (!r)
                return 0;
            return r->Fetch()[0].GetUInt32();
        }

        static uint32 CountAccountsByType(uint8 type)
        {
            QueryResult r = CharacterDatabase.Query(fmt::format("SELECT COUNT(*) FROM playerbots_accounts WHERE account_type={}", uint32(type)).c_str());
            if (!r)
                return 0;
            return r->Fetch()[0].GetUInt32();
        }

        static uint32 CalculateTotalAccountCount(uint32 explicitCount, uint32 maxRandomBots, uint32 charsPerAccount)
        {
            if (explicitCount > 0)
                return explicitCount;

            if (maxRandomBots == 0)
                return 0;

            // TrinityCore default realm character slots per account.
            charsPerAccount = std::clamp<uint32>(charsPerAccount, 1u, kMaxCharsPerAccount);
            return (maxRandomBots + charsPerAccount - 1) / charsPerAccount;
        }

        static std::string MakePassword(std::string const& username, bool randomPwd, uint32 len)
        {
            if (!randomPwd)
                return username;

            if (len < 6)
                len = 6;

            static thread_local std::mt19937 rng{ std::random_device{}() };
            std::uniform_int_distribution<int> charDist{ '!', 'z' };

            std::string out;
            out.reserve(len);
            for (uint32 i = 0; i < len; ++i)
                out.push_back(static_cast<char>(charDist(rng)));
            return out;
        }

        static bool ParseIndex(std::string const& prefix, std::string const& username, uint32& outIndex)
        {
            if (username.size() <= prefix.size())
                return false;

            // Case-insensitive prefix match (auth usernames may be stored/returned with different case).
            for (size_t i = 0; i < prefix.size(); ++i)
            {
                char a = static_cast<char>(std::tolower(static_cast<unsigned char>(username[i])));
                char b = static_cast<char>(std::tolower(static_cast<unsigned char>(prefix[i])));
                if (a != b)
                    return false;
            }

            std::string suf = username.substr(prefix.size());
            if (suf.empty())
                return false;

            for (char c : suf)
                if (!std::isdigit(static_cast<unsigned char>(c)))
                    return false;

            outIndex = static_cast<uint32>(std::stoul(suf));
            return true;
        }

        static std::vector<BotAccount> LoadAccountsByPrefix(std::string const& prefix)
        {
            std::vector<BotAccount> out;
            std::string like = prefix + "%";

            std::string sql = fmt::format("SELECT id, username FROM account WHERE username LIKE '{}'", like);
            QueryResult res = LoginDatabase.Query(sql.c_str());
            if (!res)
                return out;

            do
            {
                Field* f = res->Fetch();
                BotAccount a;
                a.Id = f[0].GetUInt32();
                a.Username = f[1].GetString();
                if (!ParseIndex(prefix, a.Username, a.Index))
                    a.Index = 0;
                out.push_back(std::move(a));
            } while (res->NextRow());

            std::sort(out.begin(), out.end(), [](BotAccount const& l, BotAccount const& r)
            {
                if (l.Index != r.Index)
                    return l.Index < r.Index;
                return l.Id < r.Id;
            });

            return out;
        }

        static void SyncPlayerbotsAccountsFromAuth(std::string const& prefix, uint32 totalAccountsNeeded)
        {
            // Source of truth: auth.account (LoginDatabase)
            // Target: characters.playerbots_accounts (CharacterDatabase)
            // This makes bootstrap idempotent and fixes cases where UpsertPoolEntry didn't persist for any reason.

            std::string like = prefix + "%";
            QueryResult res = LoginDatabase.Query(fmt::format(
                "SELECT id, username FROM account WHERE username LIKE '{}'", like).c_str());
            if (!res)
                return;

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

            uint32 upserts = 0;
            do
            {
                Field* f = res->Fetch();
                uint32 accountId = f[0].GetUInt32();
                std::string username = f[1].GetString();

                uint32 idx = 0;
                if (!ParseIndex(prefix, username, idx))
                    continue;

                if (idx >= totalAccountsNeeded)
                    continue;

                // Ensure a row exists for this accountId.
                // NOTE: account_type is left as-is if row exists; we reset/assign types later.
                trans->Append(fmt::format(
                    "INSERT INTO playerbots_accounts (account_id, account_index, account_type) "
                    "VALUES ({}, {}, 0) "
                    "ON DUPLICATE KEY UPDATE account_index=VALUES(account_index)",
                    accountId, idx).c_str());
                ++upserts;

            } while (res->NextRow());

            CharacterDatabase.CommitTransaction(trans);

            TC_LOG_INFO("server.loading", "Playerbots.RandomBots: synced playerbots_accounts from auth: upserts={} (prefix='{}' target={})",
                upserts, prefix, totalAccountsNeeded);
        }

        static uint32 CountPoolTotal()
        {
            QueryResult r = CharacterDatabase.Query("SELECT COUNT(*) FROM playerbots_accounts");
            if (!r)
                return 0;
            return r->Fetch()[0].GetUInt32();
        }

        static void UpsertPoolEntry(uint32 accountId, uint32 accountIndex)
        {
            std::string sql = fmt::format(
                "INSERT INTO playerbots_accounts (account_id, account_index, account_type) "
                "VALUES ({}, {}, 0) "
                "ON DUPLICATE KEY UPDATE account_index=VALUES(account_index)",
                accountId, accountIndex);
            CharacterDatabase.Execute(sql.c_str());
        }

        static void ResetTypesFor(std::vector<BotAccount> const& accounts)
        {
            for (auto const& a : accounts)
                CharacterDatabase.Execute(fmt::format("UPDATE playerbots_accounts SET account_type=0 WHERE account_id={}", a.Id).c_str());
        }

        static void AssignTypes(std::vector<BotAccount> const& accounts, uint32 needRnd, uint32 needAddClass)
        {
            if (accounts.empty())
                return;

            ResetTypesFor(accounts);

            // 1) Assign AddClass from the largest indices.
            uint32 assignedAdd = 0;
            for (auto it = accounts.rbegin(); it != accounts.rend() && assignedAdd < needAddClass; ++it)
            {
                CharacterDatabase.Execute(fmt::format("UPDATE playerbots_accounts SET account_type=2 WHERE account_id={}", it->Id).c_str());
                ++assignedAdd;
            }

            // 2) Assign RandomBots from the smallest indices, skipping already assigned AddClass.
            uint32 assignedRnd = 0;
            for (auto const& a : accounts)
            {
                if (assignedRnd >= needRnd)
                    break;

                QueryResult r = CharacterDatabase.Query(fmt::format(
                    "SELECT account_type FROM playerbots_accounts WHERE account_id={}", a.Id).c_str());
                if (r && r->Fetch()[0].GetUInt32() == 2)
                    continue;

                CharacterDatabase.Execute(fmt::format("UPDATE playerbots_accounts SET account_type=1 WHERE account_id={}", a.Id).c_str());
                ++assignedRnd;
            }

            TC_LOG_INFO("server.loading", "Playerbots.RandomBots: assigned types: rndbot={} addclass={}", assignedRnd, assignedAdd);
        }

        static void DeleteAccounts(std::string const& prefix)
        {
            auto accounts = LoadAccountsByPrefix(prefix);
            if (accounts.empty())
            {
                TC_LOG_INFO("server.loading", "Playerbots.RandomBots: no accounts found for prefix '{}'", prefix);
                return;
            }

            for (auto const& a : accounts)
                CharacterDatabase.Execute(fmt::format("DELETE FROM playerbots_accounts WHERE account_id={}", a.Id).c_str());

            // TrinityCore master: DeleteAccount is static and deletes characters too.
            // (core-friendly, no manual SQL on auth tables).
            for (auto const& a : accounts)
            {
                AccountOpResult r = AccountMgr::DeleteAccount(a.Id);
                if (r != AccountOpResult::AOR_OK)
                    TC_LOG_ERROR("server.loading", "Playerbots.RandomBots: DeleteAccount({}) failed (result={})", a.Id, uint32(r));
            }

            TC_LOG_WARN("server.loading", "Playerbots.RandomBots: deleted {} accounts with prefix '{}'", uint32(accounts.size()), prefix);
        }

        static std::mt19937& Rng()
        {
            // thread_local is allowed for variables at namespace scope OR static locals.
            static thread_local std::mt19937 rng{ std::random_device{}() };
            return rng;
        }

        // Avoid MSVC static_assert: uniform_int_distribution does not allow uint8_t/char types.
        static uint8 RandomU8(uint8 minVal, uint8 maxVal)
        {
            std::uniform_int_distribution<int> valueDist{ int(minVal), int(maxVal) };
            return uint8(valueDist(Rng()));
        }

        static bool IsRaceAllowed(uint8 race)
        {
            ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(race);
            if (!raceEntry)
                return false;

            if (raceEntry->GetFlags().HasFlag(ChrRacesFlag::NPCOnly))
                return false;

            if (Trinity::RaceMask<uint64> disabled{ sWorld->GetUInt64Config(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK) };
                disabled.HasRace(race))
                return false;

            RaceUnlockRequirement const* req = sObjectMgr->GetRaceUnlockRequirement(race);
            if (!req)
                return false;

            // We don't have per-account expansion here; assume server expansion capability.
            if (req->Expansion > uint8(sWorld->getIntConfig(CONFIG_EXPANSION)))
                return false;

            return true;
        }

        static bool IsClassAllowed(uint8 cls)
        {
            ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(cls);
            if (!classEntry)
                return false;

            uint32 disabledMask = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_CLASSMASK);
            if ((1u << (cls - 1)) & disabledMask)
                return false;

            return true;
        }

        // Robust/authoritative validity check:
        // Use exactly what the core considers a valid playable race/class pair.
        // This prevents "Possible hacking attempt ... invalid race/class pair" at Player::Create().
        static bool IsRaceClassAllowedByGame(uint8 race, uint8 cls)
        {
            return sObjectMgr->GetPlayerInfo(race, cls) != nullptr;
        }

        static bool PickRandomRaceClass(uint8& outRace, uint8& outClass)
        {
            std::vector<uint8> races;
            std::vector<uint8> classes;

            // Conservative ranges: 1..MAX_* (SharedDefines). We filter via DB2 lookups anyway.
            for (uint8 r = 1; r < MAX_RACES; ++r)
                if (IsRaceAllowed(r))
                    races.push_back(r);

            for (uint8 c = 1; c < MAX_CLASSES; ++c)
                if (IsClassAllowed(c))
                    classes.push_back(c);

            if (races.empty() || classes.empty())
                return false;

            std::uniform_int_distribution<size_t> rd(0, races.size() - 1);
            std::uniform_int_distribution<size_t> cd(0, classes.size() - 1);

            // Try a few combos
            for (uint32 tries = 0; tries < 200; ++tries)
            {
                uint8 race = races[rd(Rng())];
                uint8 cls  = classes[cd(Rng())];
                if (!IsRaceClassAllowedByGame(race, cls))
                    continue;

                outRace = race;
                outClass = cls;
                return true;
            }

            return false;
        }

        static void GenerateValidCustomizations(WorldSession& session, uint8 race, uint8 cls, uint8 sex,
            decltype(WorldPackets::Character::CharacterCreateInfo::Customizations)& out)
        {
            // MVP (scripts-friendly): keep empty customizations and rely on core defaults.
            // This avoids depending on internal DB2Manager helper APIs not exposed to scripts target.
            out.clear();

            // If empty is not accepted on your build, we will extend this in the next portion
            // by including the real header that owns DB2Manager on your branch.
            (void)session;
            (void)race;
            (void)cls;
            (void)sex;
        }

        static bool CreateCharacterForAccount(uint32 accountId, std::string const& name)
        {
            if (name.empty())
                return false;

            // Safety net: even if the pool query is supposed to exclude existing names,
            // do a direct check because the name column is UNIQUE.
            if (CharacterNameExists(name))
            {
                TC_LOG_WARN("server.loading", "Playerbots.RandomBots: name '{}' already exists in characters table. Skipping.", name);
                return false;
            }

            uint8 race = RACE_NONE;
            uint8 cls = CLASS_NONE;
            if (!PickRandomRaceClass(race, cls))
                return false;

            uint8 sex = RandomU8(0, 1);

            uint8 level = PickBotLevel();
            if (level < 1)
                level = 1;

            // Build an "offline" session for creation + ValidateAppearance.
            // TrinityCore master retail WorldSession ctor expects many params (see WorldSession.h/.cpp).
            std::shared_ptr<WorldSocket> sock; // null socket (offline)

            ClientBuild::VariantId buildVariant{};
            buildVariant.Platform = 0;
            buildVariant.Arch = 0;
            buildVariant.Type = ClientBuild::Type::Retail;

            std::shared_ptr<WorldSession> session = std::make_shared<WorldSession>(
                accountId,
                std::string("rndbot"),              // account name (not critical for offline creation)
                0u,                                 // battlenetAccountId
                std::string(),                       // battlenetAccountEmail
                std::move(sock),
                AccountTypes::SEC_PLAYER,
                uint8(sWorld->getIntConfig(CONFIG_EXPANSION)),
                0,                                  // mute_time
                std::string("Playerbots"),           // os
                Minutes(0),                          // timezone offset
                0u,                                  // build
                buildVariant,
                LOCALE_enUS,
                0u,                                  // recruiter
                false);                              // is recruiter

            // Headless bot session: disable pointless packet sending/log spam in core.
            session->SetHeadlessBotSession(true);

            auto createInfo = std::make_shared<WorldPackets::Character::CharacterCreateInfo>();
            createInfo->Race = race;
            createInfo->Class = cls;
            createInfo->Sex = sex;
            createInfo->Name = name;

            // Level 1: optionally start in NPE (Exile's Reach) if the core data provides NPE start for this race/class.
            // IMPORTANT: Do not force NPE when createPositionNPE is missing (core will fallback anyway, but keep it explicit).
            if (level == 1)
            {
                PlayerInfo const* info = sObjectMgr->GetPlayerInfo(race, cls);
                if (info && info->createPositionNPE)
                {
                    uint32 chance = sConfigMgr->GetIntDefault("Playerbots.RandomBots.Level1.UseNPEChancePct", 50);
                    if (chance > 100)
                        chance = 100;
                    createInfo->UseNPE = (urand(0, 99) < chance);
                }
            }

            GenerateValidCustomizations(*session, race, cls, sex, createInfo->Customizations);

            // Validate with possibly-empty customizations.
            UF::ChrCustomizationChoice const* beginPtr = createInfo->Customizations.empty() ? nullptr : createInfo->Customizations.data();
            UF::ChrCustomizationChoice const* endPtr   = createInfo->Customizations.empty() ? nullptr : (createInfo->Customizations.data() + createInfo->Customizations.size());

            if (!session->ValidateAppearance(Races(race), Classes(cls), Gender(sex),
                Trinity::IteratorPair<UF::ChrCustomizationChoice const*>(beginPtr, endPtr)))
            {
                createInfo->Customizations.clear();
            }

            std::shared_ptr<Player> newChar(new Player(session.get()), [](Player* p)
            {
                p->CleanupsBeforeDelete();
                delete p;
            });

            newChar->GetMotionMaster()->Initialize();

            // TrinityCore retail: Player::Create expects ObjectGuid::LowType (see CharacterHandler.cpp).
            ObjectGuid::LowType guidLow = GenerateFreeCharacterGuidLow();
            if (!newChar->Create(guidLow, createInfo.get()))
                return false;

            if (level > 1)
            {
                // Minimal leveling (gear/talents handled in later milestones).
                newChar->GiveLevel(level);
                newChar->SetXP(0);
            }

            newChar->SetAtLoginFlag(AT_LOGIN_FIRST);

            CharacterDatabaseTransaction characterTransaction = CharacterDatabase.BeginTransaction();
            LoginDatabaseTransaction loginTransaction = LoginDatabase.BeginTransaction();

            CleanupCharacterRowsForGuid(guidLow, characterTransaction);

            newChar->SaveToDB(loginTransaction, characterTransaction, true);

            CharacterDatabase.CommitTransaction(characterTransaction);
            LoginDatabase.CommitTransaction(loginTransaction);

            // SaveToDB can fail (e.g. duplicate name). Confirm persistence before registering anything.
            if (!CharacterGuidExists(guidLow))
            {
                TC_LOG_ERROR("server.loading",
                    "Playerbots.RandomBots: SaveToDB failed for '{}' (acc={} guid={}). Character row not found after commit.",
                    name, accountId, uint32(guidLow));
                return false;
            }

            // Optional relocation for non-level-1 bots (controlled by spawn hubs table).
            if (level > 1)
            {
                uint8 teamId = uint8(newChar->GetTeamId());
                if (Optional<SpawnHub> hub = PickSpawnHubFor(teamId, level))
                {
                    // DB-only relocation (no MapManager/Map API usage).
                    // This compiles across TC variants and avoids relying on map helpers during bootstrap.
                    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

                    trans->Append(fmt::format(
                        "UPDATE characters "
                        "SET map={}, zone=0, position_x={}, position_y={}, position_z={}, orientation={} "
                        "WHERE guid={}",
                        uint32(hub->MapId), hub->X, hub->Y, hub->Z, hub->O, uint32(guidLow)).c_str());

                    // Optional: update homebind if the table exists and an AreaId is provided in the hub row.
                    // We intentionally do not attempt to compute zone/area via Map API here.
                    if (hub->AreaId && TableExistsInCharacters("character_homebind"))
                    {
                        trans->Append(fmt::format(
                            "REPLACE INTO character_homebind (guid, mapId, zoneId, posX, posY, posZ, orientation) "
                            "VALUES ({}, {}, {}, {}, {}, {}, {})",
                            uint32(guidLow), uint32(hub->MapId), uint32(hub->AreaId),
                            hub->X, hub->Y, hub->Z, hub->O).c_str());
                    }

                    CharacterDatabase.CommitTransaction(trans);

                    // Keep the in-memory player object consistent for the rest of this function.
                    newChar->Relocate(hub->X, hub->Y, hub->Z, hub->O);
                }
            }

            sCharacterCache->AddCharacterCacheEntry(newChar->GetGUID(), accountId, newChar->GetName(),
                newChar->GetNativeGender(), newChar->GetRace(), newChar->GetClass(), newChar->GetLevel(), false);

            // Register in playerbots_bots as RandomBot (upsert, so reruns are idempotent).
            CharacterDatabase.Execute(fmt::format(
                "INSERT INTO playerbots_bots (guid, bot_type, owner_guid) VALUES ({}, {}, 0) "
                "ON DUPLICATE KEY UPDATE bot_type=VALUES(bot_type), owner_guid=VALUES(owner_guid)",
                newChar->GetGUID().GetCounter(), uint32(BOT_TYPE_RANDOM)).c_str());

            TC_LOG_INFO("server.loading", "Playerbots.RandomBots: created char '{}' acc={} guid={} race={} class={} sex={} level={} createMode={} customizations={}",
                newChar->GetName(), accountId, newChar->GetGUID().ToString(), uint32(race), uint32(cls), uint32(sex), uint32(newChar->GetLevel()),
                uint32(newChar->GetCreateMode()), uint32(createInfo->Customizations.size()));

            return true;
        }

        static uint32 CountCharactersInRandomPool()
        {
            // Count only RandomBots characters by linking characters.account to playerbots_accounts(account_type=1).
            // This does NOT depend on playerbots_bots being in sync and does not require any schema change in characters.
            QueryResult r = CharacterDatabase.Query(
                "SELECT COUNT(*) "
                "FROM characters c "
                "INNER JOIN playerbots_accounts a ON a.account_id = c.account "
                "WHERE a.account_type = 1");
            if (!r)
                return 0;
            return r->Fetch()[0].GetUInt32();
        }

        static void ReconcilePlayerbotsBotsTable()
        {
            if (!TableExistsInCharacters("playerbots_bots"))
                return;

            // 1) Remove orphans
            CharacterDatabase.Execute(
                "DELETE b FROM playerbots_bots b "
                "LEFT JOIN characters c ON c.guid=b.guid "
                "WHERE c.guid IS NULL");

            // 2) Recreate missing entries for random pool characters
            CharacterDatabase.Execute(fmt::format(
                "INSERT INTO playerbots_bots (guid, bot_type, owner_guid) "
                "SELECT c.guid, {}, 0 "
                "FROM characters c "
                "JOIN playerbots_accounts a ON a.account_id=c.account "
                "LEFT JOIN playerbots_bots b ON b.guid=c.guid "
                "WHERE a.account_type=1 AND b.guid IS NULL",
                uint32(BOT_TYPE_RANDOM)).c_str());
        }

        static uint32 ComputeDesiredRandomBots(uint32 maxRandomBots, uint32 rndAccounts, uint32 perAccount)
        {
            if (maxRandomBots > 0)
                return maxRandomBots;

            if (rndAccounts == 0 || perAccount == 0)
                return 0;

            // Fallback objective if MaxRandomBots is not set: fill all rndbot accounts up to perAccount.
            return rndAccounts * perAccount;
        }

        static void CreateCharactersForPool(uint32 perAccount, uint32 toCreate, uint32 maxAttemptsPerChar)
        {
            maxAttemptsPerChar = std::max<uint32>(1, maxAttemptsPerChar);

            // Only for type=1 accounts (rndbot). AddClass will be used later.
            QueryResult r = CharacterDatabase.Query("SELECT account_id FROM playerbots_accounts WHERE account_type=1 ORDER BY account_index ASC");
            if (!r)
                return;

            uint32 created = 0;
            do
            {
                uint32 accountId = r->Fetch()[0].GetUInt32();
                uint32 have = CountCharactersForAccount(accountId);
                if (have >= perAccount)
                    continue;

                bool createdThisAccount = false;
                for (uint32 attempt = 1; attempt <= maxAttemptsPerChar; ++attempt)
                {
                    std::string name = PickFreeNameFromPool();
                    if (name.empty())
                    {
                        TC_LOG_ERROR("server.loading", "Playerbots.RandomBots: name pool exhausted (no free names).");
                        return;
                    }

                    if (CreateCharacterForAccount(accountId, name))
                    {
                        ++created;
                        createdThisAccount = true;
                        break;
                    }

                    TC_LOG_WARN("server.loading",
                        "Playerbots.RandomBots: CreateCharacter failed for acc={} (attempt {}/{}). Retrying with another name.",
                        accountId, attempt, maxAttemptsPerChar);
                }

                if (!createdThisAccount)
                {
                    TC_LOG_ERROR("server.loading",
                        "Playerbots.RandomBots: giving up creating a character for acc={} after {} attempts.",
                        accountId, maxAttemptsPerChar);
                }
                if (created >= toCreate)
                    break;
                // Not enough free name / creation failures can happen. Continue other accounts.
                if (!createdThisAccount)
                     continue;

            } while (r->NextRow());

            TC_LOG_INFO("server.loading", "Playerbots.RandomBots: CreateCharacters done. createdThisBoot={}", created);
        }

        static std::vector<BotAccount> FilterAccountsToTarget(std::vector<BotAccount> const& all, uint32 totalAccountsNeeded)
        {
            std::vector<BotAccount> out;
            out.reserve(std::min<uint32>(totalAccountsNeeded, uint32(all.size())));

            for (BotAccount const& a : all)
            {
                if (a.Index < totalAccountsNeeded)
                    out.push_back(a);
            }

            std::sort(out.begin(), out.end(), [](BotAccount const& l, BotAccount const& r)
            {
                if (l.Index != r.Index)
                    return l.Index < r.Index;
                return l.Id < r.Id;
            });

            return out;
        }
    }

    void OnStartup()
    {
        if (!sConfigMgr->GetBoolDefault("Playerbots.RandomBots.Enable", false))
            return;

        // Require the pool table. If missing, skip instead of crashing startup.
        if (!TableExistsInCharacters("playerbots_accounts"))
        {
            TC_LOG_ERROR("server.loading", "Playerbots.RandomBots: table 'playerbots_accounts' is missing in characters DB. Skipping bootstrap.");
            return;
        }

        std::string prefix = sConfigMgr->GetStringDefault("Playerbots.RandomBots.AccountPrefix", "rndbot");
        uint32 explicitCount = sConfigMgr->GetIntDefault("Playerbots.RandomBots.AccountCount", 0);
        uint32 maxRandomBots = sConfigMgr->GetIntDefault("Playerbots.RandomBots.MaxRandomBots", 0);
        uint32 addClassPool = sConfigMgr->GetIntDefault("Playerbots.RandomBots.AddClassAccountPoolSize", 0);
        bool randomPwd = sConfigMgr->GetBoolDefault("Playerbots.RandomBots.RandomPassword", false);
        uint32 pwdLen = sConfigMgr->GetIntDefault("Playerbots.RandomBots.RandomPasswordLength", 10);
        bool deleteAccounts = sConfigMgr->GetBoolDefault("Playerbots.RandomBots.DeleteAccounts", false);
		
        // Used both for account sizing and for character creation.
        uint32 perAccount = sConfigMgr->GetIntDefault("Playerbots.RandomBots.CharactersPerAccount", 10);
        if (perAccount > kMaxCharsPerAccount)
            perAccount = kMaxCharsPerAccount;
        perAccount = std::max<uint32>(1, perAccount);


        if (deleteAccounts)
        {
            DeleteAccounts(prefix);
            return;
        }

        uint32 total = CalculateTotalAccountCount(explicitCount, maxRandomBots, perAccount);
        if (total == 0)
        {
            TC_LOG_INFO("server.loading", "Playerbots.RandomBots: nothing to do (AccountCount=0 and MaxRandomBots=0).");
            return;
        }

        TC_LOG_INFO("server.loading", "Playerbots.RandomBots: bootstrap start. prefix='{}' totalAccounts={}", prefix, total);

        for (uint32 i = 0; i < total; ++i)
        {
            std::string username = prefix + std::to_string(i);
            uint32 accountId = AccountMgr::GetId(username);

            if (!accountId)
            {
                std::string password = MakePassword(username, randomPwd, pwdLen);

                // TrinityCore master: CreateAccount is a member function.
                AccountOpResult r = sAccountMgr->CreateAccount(username, password, "");
                if (r != AccountOpResult::AOR_OK)
                {
                    TC_LOG_ERROR("server.loading", "Playerbots.RandomBots: CreateAccount('{}') failed (result={})", username, uint32(r));
                    continue;
                }

                accountId = AccountMgr::GetId(username);
            }

            if (accountId)
                UpsertPoolEntry(accountId, i);
        }

        // Hard sync pass (fixes "playerbots_accounts ended up empty" no matter what happened above)
        SyncPlayerbotsAccountsFromAuth(prefix, total);

        // If still empty, we can't proceed.
        if (CountPoolTotal() == 0)
        {
            TC_LOG_ERROR("server.loading", "Playerbots.RandomBots: playerbots_accounts is empty after sync. Check characters DB connection/schema.");
            return;
        }

        auto allAccounts = LoadAccountsByPrefix(prefix);
        auto accounts = FilterAccountsToTarget(allAccounts, total);

        // Minimal assignment policy (ACore-like ordering):
        // - RNDbot from smallest indices
        // - AddClass from largest indices
        uint32 totalExisting = std::min<uint32>(total, uint32(accounts.size()));
        uint32 needAdd = std::min<uint32>(addClassPool, totalExisting);
        uint32 needRnd = (totalExisting > needAdd) ? (totalExisting - needAdd) : 0;
        AssignTypes(accounts, needRnd, needAdd);

        // Phase B1 preflight: we will create level 1 characters later, using core creation pipeline.
        bool createChars = sConfigMgr->GetBoolDefault("Playerbots.RandomBots.CreateCharacters", maxRandomBots > 0);
        uint32 maxAttemptsPerChar = sConfigMgr->GetIntDefault("Playerbots.RandomBots.CreateCharactersMaxAttemptsPerChar", 10);
        maxAttemptsPerChar = std::max<uint32>(1, maxAttemptsPerChar);

        if (createChars)
        {
            if (!TableExistsInCharacters("playerbots_names"))
            {
                TC_LOG_ERROR("server.loading", "Playerbots.RandomBots: CreateCharacters=1 but table 'playerbots_names' is missing. Skipping.");
            }
            else
            {
                // Safety: ensure pool exists before counting/creating.
                if (CountPoolTotal() == 0)
                {
                    TC_LOG_WARN("server.loading", "Playerbots.RandomBots: playerbots_accounts empty at CreateCharacters stage, resyncing...");
                    SyncPlayerbotsAccountsFromAuth(prefix, total);
                    if (CountPoolTotal() == 0)
                    {
                        TC_LOG_ERROR("server.loading", "Playerbots.RandomBots: still empty after resync. Aborting creation.");
                        return;
                    }
                }

                uint32 freeNames = CountNamePoolFree();
                uint32 rndAccounts = CountAccountsByType(1);
                uint32 addAccounts = CountAccountsByType(2);

                TC_LOG_INFO("server.loading",
                    "Playerbots.RandomBots: CreateCharacters preflight: perAccount={} freeNames={} accounts(rndbot={}/addclass={})",
                    perAccount, freeNames, rndAccounts, addAccounts);

                // Self-heal playerbots_bots when wiped or desynced.
                ReconcilePlayerbotsBotsTable();

                // Create ONLY what is missing according to desired target.
                uint32 currentBots = CountCharactersInRandomPool();
                uint32 desiredBots = ComputeDesiredRandomBots(maxRandomBots, rndAccounts, perAccount);

                if (desiredBots == 0)
                {
                    TC_LOG_INFO("server.loading", "Playerbots.RandomBots: desiredBots=0, skipping creation.");
                }
                else if (currentBots >= desiredBots)
                {
                    TC_LOG_INFO("server.loading", "Playerbots.RandomBots: bots target reached (current={} desired={}), skipping creation.", currentBots, desiredBots);
                }
                else
                {
                    uint32 missing = desiredBots - currentBots;
                    TC_LOG_INFO("server.loading", "Playerbots.RandomBots: creating missing bots (current={} desired={} missing={})",
                        currentBots, desiredBots, missing);

                    // Create all missing bots in a single boot (as requested).
                    CreateCharactersForPool(perAccount, missing, maxAttemptsPerChar);
                }
            }
        }

        TC_LOG_INFO("server.loading", "Playerbots.RandomBots: bootstrap done.");
    }
}