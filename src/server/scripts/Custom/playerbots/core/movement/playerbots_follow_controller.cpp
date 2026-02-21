#include "playerbots_follow_controller.h"

#include "Log.h"
#include <cmath>
#include "MotionMaster.h"
#include "Player.h"

namespace Playerbots
{
    namespace
    {
        // A dedicated point id so we can identify our own motion.
        constexpr uint32 PB_FOLLOW_POINT_ID = 0x5042464F; // 'PBFO'
    }

    FollowController& FollowController::Instance()
    {
        static FollowController inst;
        return inst;
    }

    void FollowController::Stop(Player* bot)
    {
        if (!bot)
            return;

        // Don't nuke all motion (too intrusive). Just idle.
        bot->GetMotionMaster()->MoveIdle();
    }

    bool FollowController::ComputeAnchorPoint(Player* bot, Player* leader, float dist, float angle, float& outX, float& outY, float& outZ) const
    {
        if (!bot || !leader)
            return false;

        float x = leader->GetPositionX();
        float y = leader->GetPositionY();
        float z = leader->GetPositionZ();

        // IMPORTANT: Formation angles are in MoveFollow convention (0 = behind).
        // WorldObject::GetClosePoint uses relAngle where 0 = in front (orientation), PI = behind.
        // Convert: moveFollowAngle -> relAngle.
        float const relAngle = angle + float(M_PI);

        // GetClosePoint accounts for collision radius.
        leader->GetClosePoint(x, y, z, bot->GetCombatReach(), dist, relAngle);

        outX = x;
        outY = y;
        outZ = z;
        return true;
    }

    void FollowController::EnsureMotionTo(Player* bot, float x, float y, float z)
    {
        if (!bot)
            return;

        // MovePoint will naturally adjust speed; it also avoids the chase stop/run oscillation.
        bot->GetMotionMaster()->MovePoint(PB_FOLLOW_POINT_ID, x, y, z);
    }

    void FollowController::Update(Player* bot, Player* leader, float dist, float angle, FollowTuning const& tuning)
    {
        if (!bot || !leader || bot == leader)
            return;

        if (!bot->IsInWorld() || !leader->IsInWorld())
            return;

        // If leader not in same map/instance, the manager will handle far teleport (TeleportNear + ACK).
        if (bot->GetMapId() != leader->GetMapId() || bot->GetInstanceId() != leader->GetInstanceId())
            return;

        float const d = bot->GetDistance(leader);

        // If extremely far on same map (lag spike), snap close to avoid long path catch-up.
        if (d > tuning.TeleportDist)
        {
            float sx, sy, sz;
            if (ComputeAnchorPoint(bot, leader, dist, angle, sx, sy, sz))
            {
                bot->NearTeleportTo(sx, sy, sz, leader->GetOrientation(), false);
                TC_LOG_DEBUG("playerbots", "Playerbots: follow snap NearTeleportTo (same map) bot={} dist={}", bot->GetName(), d);
            }
            return;
        }

        // Compute target point relative to leader.
        float tx, ty, tz;
        if (!ComputeAnchorPoint(bot, leader, dist, angle, tx, ty, tz))
            return;

        // Enforce configured distance more strictly (otherwise bots can stabilize "close enough").
        float const distError = std::fabs(d - dist);

        // Deadzone: if already close to target, don't constantly reset MovePoint.
        float const toTarget = bot->GetDistance(tx, ty, tz);
        if (toTarget <= tuning.Deadzone && d <= tuning.ResyncDist && distError <= 0.35f)
             return;

        EnsureMotionTo(bot, tx, ty, tz);
    }
}