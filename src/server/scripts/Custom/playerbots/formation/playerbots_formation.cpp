/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots_formation.h"

#include "ObjectGuid.h"

#include <cmath>

namespace
{
    // Deterministic small hash for stable "random" offsets (Chaos/Near).
    static uint32 StableHash(ObjectGuid const& guid)
    {
        // ObjectGuid has operator< and internal representation; ToString is stable.
        // Keep it cheap: xor-fold a few chars.
        std::string const s = guid.ToString();
        uint32 h = 2166136261u;
        for (char c : s)
            h = (h ^ uint8(c)) * 16777619u;
        return h;
    }

    static float Frac01(uint32 h)
    {
        // [0..1)
        return (h & 0xFFFFFF) / float(0x1000000);
    }
}

namespace Playerbots::Formation
{
    char const* ToString(Type type)
    {
        switch (type)
        {
            case Type::Arrow:  return "Arrow";
            case Type::Queue:  return "Queue";
            case Type::Near:   return "Near";
            case Type::Melee:  return "Melee";
            case Type::Line:   return "Line";
            case Type::Circle: return "Circle";
            case Type::Chaos:  return "Chaos";
            case Type::Shield: return "Shield";
            default:           return "Unknown";
        }
    }

    FollowParams Compute(Type type, float baseDist, float baseAngle, uint32 index, uint32 total, ObjectGuid const& botGuid)
    {
        FollowParams p;
        p.Dist = baseDist;
        p.Angle = baseAngle;

        // Common tuning knobs (purely code-level for MVP).
        constexpr float kSide = 0.35f;      // lateral spread (radians)
        constexpr float kStep = 1.25f;      // distance increment
        constexpr float kTight = 0.18f;     // small spread (radians)
        constexpr float kTwoPi = 6.28318530718f;

        // Helper: alternating left/right around 0: 0, +1, -1, +2, -2, ...
        auto altIndex = [](uint32 i) -> int32
        {
            if (i == 0)
                return 0;
            int32 k = int32((i + 1) / 2);
            return (i % 2) ? k : -k;
        };

        switch (type)
        {
            case Type::Queue:
            {
                // Single file behind leader.
                p.Dist = baseDist + float(index) * kStep;
                p.Angle = baseAngle;
                break;
            }
            case Type::Arrow:
            {
                // V-shape: distance increases, angle alternates left/right.
                int32 ai = altIndex(index);
                p.Dist = baseDist + std::abs(ai) * kStep;
                p.Angle = baseAngle + float(ai) * kSide;
                break;
            }
            case Type::Line:
            {
                // Spread behind leader, mostly same distance.
                int32 ai = altIndex(index);
                p.Dist = baseDist + float(std::abs(ai)) * 0.25f;
                p.Angle = baseAngle + float(ai) * kSide;
                break;
            }
            case Type::Circle:
            {
                // Evenly distributed around leader.
                if (total == 0)
                    break;
                float t = float(index) / float(total);
                p.Angle = baseAngle + t * kTwoPi;
                p.Dist = std::max(2.0f, baseDist);
                break;
            }
            case Type::Shield:
            {
                // Semi-circle in front of leader (front ~= baseAngle + PI).
                // Spread across +/- 90°.
                if (total == 0)
                    break;
                float t = (total == 1) ? 0.5f : float(index) / float(total - 1); // [0..1]
                float front = baseAngle + 3.14159265359f;
                float arc = (t - 0.5f) * 3.14159265359f; // [-pi/2..+pi/2]
                p.Angle = front + arc;
                p.Dist = std::max(2.0f, baseDist);
                break;
            }
            case Type::Melee:
            {
                // Tight pack around behind/sides.
                int32 ai = altIndex(index);
                p.Dist = std::max(1.5f, baseDist * 0.85f);
                p.Angle = baseAngle + float(ai) * kTight;
                break;
            }
            case Type::Near:
            {
                // Stay close; slight stable jitter.
                uint32 h = StableHash(botGuid);
                float a = (Frac01(h) - 0.5f) * 2.0f * kTight;
                float d = (Frac01(h >> 8) - 0.5f) * 0.35f;
                p.Dist = std::max(1.5f, baseDist + d);
                p.Angle = baseAngle + a;
                break;
            }
            case Type::Chaos:
            {
                // Larger stable jitter in both angle and distance.
                uint32 h = StableHash(botGuid);
                float a = (Frac01(h) - 0.5f) * 2.0f * 1.2f; // ~ +/-1.2 rad
                float d = (Frac01(h >> 8) - 0.5f) * 4.0f;  // +/-2 yards
                p.Dist = std::max(1.5f, baseDist + d);
                p.Angle = baseAngle + a;
                break;
            }
        }

        return p;
    }
}