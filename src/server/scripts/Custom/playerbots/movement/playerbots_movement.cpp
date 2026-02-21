/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots_movement.h"

#include "MotionMaster.h"
#include "Log.h"
#include "Optional.h"
#include "Player.h"
#include "Unit.h"
#include "WorldSession.h"

namespace Playerbots::Movement
{
    void Follow(Unit* follower, Player* leader, float dist, float angle)
    {
        if (!follower || !leader || follower == leader)
            return;

        follower->GetMotionMaster()->MoveFollow(leader, dist, angle);
    }

    void Unfollow(Unit* unit)
    {
        if (!unit)
            return;

        unit->StopMoving();
        unit->GetMotionMaster()->Clear(MOTION_SLOT_ACTIVE);

        if (unit->IsCreature())
            unit->GetMotionMaster()->MoveTargetedHome();
        else
            unit->GetMotionMaster()->MoveIdle();
    }

    void Stop(Unit* unit)
    {
        if (!unit)
            return;

        unit->StopMoving();
        unit->GetMotionMaster()->Clear(MOTION_SLOT_ACTIVE);
        unit->GetMotionMaster()->MoveIdle();
    }

    bool TeleportNear(Player* bot, Player* master, float dist, float angle)
    {
        if (!bot || !master || bot == master)
            return false;

        // Very verbose debug logs to diagnose headless/session/instance edge cases.
        TC_LOG_INFO("playerbots",
            "Playerbots: TeleportNear request bot={}({}) master={}({}) botMap={} botInst={} masterMap={} masterInst={} botInWorld={} masterInWorld={} botAlive={} masterAlive={} botCombat={} masterCombat={} botFlight={} masterFlight={} botBeingTeleported={}",
            bot->GetName(), bot->GetGUID().ToString(),
            master->GetName(), master->GetGUID().ToString(),
            bot->GetMapId(), bot->GetInstanceId(),
            master->GetMapId(), master->GetInstanceId(),
            bot->IsInWorld(), master->IsInWorld(),
            bot->IsAlive(), master->IsAlive(),
            bot->IsInCombat(), master->IsInCombat(),
            bot->IsInFlight(), master->IsInFlight(),
            bot->IsBeingTeleported());

        if (!bot->IsInWorld() || !master->IsInWorld())
        {
            TC_LOG_INFO("playerbots", "Playerbots: TeleportNear denied (not in world).");
            return false;
        }

        if (!bot->IsAlive() || !master->IsAlive())
        {
            TC_LOG_INFO("playerbots", "Playerbots: TeleportNear denied (dead).");
            return false;
        }

        // For safety: don't teleport while bot is in flight or already teleporting.
        if (bot->IsBeingTeleported())
        {
            TC_LOG_INFO("playerbots", "Playerbots: TeleportNear denied (bot is being teleported).");
            return false;
        }

        if (bot->IsInFlight())
        {
            TC_LOG_INFO("playerbots", "Playerbots: TeleportNear denied (bot in flight).");
            return false;
        }

        // Clamp distance.
        if (dist < 0.0f)
            dist = 0.0f;

        float x = master->GetPositionX();
        float y = master->GetPositionY();
        float z = master->GetPositionZ();
        master->GetClosePoint(x, y, z, bot->GetCombatReach(), dist, angle);

        // Same map: use direct server-side relocation instead of NearTeleportTo.
        //
        // WHY: On TrinityCore retail (TWW+), NearTeleportTo sends SMSG_MOVE_TELEPORT and then
        // waits for a CMSG_MOVE_TELEPORT_ACK from the client before actually relocating the player.
        // Headless bots have no socket, so the ACK never arrives, and the bot gets permanently
        // stuck with IsBeingTeleported() == true.
        //
        // Fix: bypass the packet/ACK system entirely by calling Relocate() directly.
        // This is safe for headless bots because:
        //   - Relocate() updates the WorldObject position (used for distance checks, AI, etc.)
        //   - m_movementInfo.pos.Relocate() updates the movement packet position
        //   - StopMoving() clears any in-progress spline/path
        //   - UpdateObjectVisibility() notifies nearby players via SMSG_UPDATE_OBJECT
        if (bot->GetMapId() == master->GetMapId() && bot->GetInstanceId() == master->GetInstanceId())
        {
            bot->StopMoving();
            bot->GetMotionMaster()->Clear();

            // Update internal WorldObject position (grid/distance checks use this)
            bot->Relocate(x, y, z, master->GetOrientation());

            // Update the movement info position (used for outgoing movement packets to other clients)
            bot->m_movementInfo.pos.Relocate(x, y, z, master->GetOrientation());

            // Notify nearby players that this bot has moved
            bot->UpdateObjectVisibility(true);

            TC_LOG_INFO("playerbots",
                "Playerbots: TeleportNear used direct Relocate (same map/instance). destPos={:.3f},{:.3f},{:.3f}",
                x, y, z);
            return true;
        }

        // IMPORTANT: force master's instanceId when relevant (instances/raids).
        Optional<uint32> destInstanceId = {};
        if (master->GetInstanceId() != 0)
            destInstanceId = master->GetInstanceId();

        bool ok = bot->TeleportTo(master->GetMapId(), x, y, z, master->GetOrientation(), TELE_TO_NONE, destInstanceId);

        // Headless bots will never send MSG_MOVE_WORLDPORT_ACK.
        // On cross-map teleports, TeleportTo schedules the transfer and waits for that ACK to finalize.
        // For headless sessions, we must finalize server-side.
        if (ok)
        {
            WorldSession* session = bot->GetSession();
            if (session && session->IsHeadlessBotSession())
            {
                // Only needed for far teleports (map change / worldport).
                if (bot->IsBeingTeleportedFar())
                {
                    TC_LOG_INFO("playerbots",
                        "Playerbots: TeleportNear finalizing far teleport via HandleMoveWorldportAck (headless).");

                    session->HandleMoveWorldportAck();

                    TC_LOG_INFO("playerbots",
                        "Playerbots: TeleportNear post-ack botMap={} botInst={} botPos={:.3f},{:.3f},{:.3f}",
                        bot->GetMapId(), bot->GetInstanceId(),
                        bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());
                }
            }
        }

        return ok;
    }
}