#include "ScriptMgr.h"

#include "ObjectAccessor.h"
#include "Player.h"
#include "Group.h"

#include "playerbots/core/playerbots_manager.h"

namespace Playerbots
{
    class PlayerbotsLifecycle_PlayerScript : public PlayerScript
    {
    public:
        PlayerbotsLifecycle_PlayerScript() : PlayerScript("PlayerbotsLifecycle_PlayerScript") { }

        // TrinityCore (your branch): OnLogin(Player*, bool)
        void OnLogin(Player* player, bool /*firstLogin*/) override
        {
            if (!player)
                return;
            Manager::Instance().OnMasterAvailable(player);
        }

        void OnLogout(Player* player) override
        {
            if (!player)
                return;
            Manager::Instance().OnMasterUnavailable(player);
        }
    };

    class PlayerbotsLifecycle_GroupScript : public GroupScript
    {
    public:
        PlayerbotsLifecycle_GroupScript() : GroupScript("PlayerbotsLifecycle_GroupScript") { }

        // This signature is stable across many TC versions.
        void OnRemoveMember(Group* group, ObjectGuid guid, RemoveMethod /*method*/, ObjectGuid /*kicker*/, char const* /*reason*/) override
        {
            if (!group)
                return;

            Manager::Instance().OnMasterUnavailable(guid);
        }

        void OnDisband(Group* group) override
        {
            if (!group)
                return;

            // TrinityCore: iterate via GetMemberSlots()
            for (Group::MemberSlot const& slot : group->GetMemberSlots())
            {
                Manager::Instance().OnMasterUnavailable(slot.guid);
            }
        }
    };
}

void AddSC_playerbots_lifecycle_script()
{
    new Playerbots::PlayerbotsLifecycle_PlayerScript();
    new Playerbots::PlayerbotsLifecycle_GroupScript();
}