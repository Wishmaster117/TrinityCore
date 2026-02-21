/*
 * Playerbots - follow controller (formation point follower)
 *
 * This keeps bot movement smooth by driving MovePoint to a moving anchor point
 * relative to the leader, instead of relying on MoveFollow chase behavior.
 */

#ifndef PLAYERBOTS_FOLLOW_CONTROLLER_H
#define PLAYERBOTS_FOLLOW_CONTROLLER_H

#include <cstdint>

class Player;

namespace Playerbots
{
    struct FollowTuning
    {
        float BaseDist = 5.0f;            // Configurable follow distance
        float BaseAngle = 3.14159265f;    // Behind leader by default
        float Deadzone = 1.0f;            // Don't update target if leader point barely moved
        float ResyncDist = 12.0f;         // If too far, force refresh
        float TeleportDist = 60.0f;       // If on same map and too far, use NearTeleportTo
        uint32 UpdateMs = 250;            // Target refresh rate
    };

    class FollowController
    {
    public:
        static FollowController& Instance();

        void Update(Player* bot, Player* leader, float dist, float angle, FollowTuning const& tuning);
        void Stop(Player* bot);

    private:
        FollowController() = default;

        bool ComputeAnchorPoint(Player* bot, Player* leader, float dist, float angle, float& outX, float& outY, float& outZ) const;
        void EnsureMotionTo(Player* bot, float x, float y, float z);
    };
}

#endif // PLAYERBOTS_FOLLOW_CONTROLLER_H