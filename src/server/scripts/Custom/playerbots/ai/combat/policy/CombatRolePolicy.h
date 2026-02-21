#ifndef PLAYERBOTS_AI_COMBAT_POLICY_COMBATROLEPOLICY_H
#define PLAYERBOTS_AI_COMBAT_POLICY_COMBATROLEPOLICY_H

#include <cstdint>

namespace Playerbots::AI::Combat::Policy
{
    enum class Role : uint8
    {
        Tank   = 0,
        Healer = 1,
        Dps    = 2
    };

    struct Settings
    {
        // Movement
        bool KeepRangedDistance = false;
        float PreferredRangeYards = 0.0f; // 0 => melee chase

        // Threat / tanking
        bool AllowTaunt = false;

        // Thresholds
        uint8 HealThresholdPct = 85;   // heal if target <= this pct
        uint8 SelfHealPct = 60;
        uint8 MasterHealPct = 70;

        // Dispel scope
        bool DispelGroup = false;

        // AOE
        uint8 AoeMinTargets = 3;
    };

    class CombatRolePolicy final
    {
    public:
        static Role DetermineRole(bool inTankMode, bool hasTaunt, bool hasMelee, bool hasRangedOffense,
                                  bool hasHeals, uint32 healCount, uint32 offensiveCount)
        {
            // 1) Tank: explicit tank mode OR clear melee+tank toolkit and not a caster
            if (inTankMode || (hasTaunt && hasMelee && !hasRangedOffense))
                return Role::Tank;

            // 2) Healer: has heals and heal kit dominates offensive kit (generic heuristic)
            if (hasHeals && healCount >= (offensiveCount + 1))
                return Role::Healer;

            return Role::Dps;
        }

        static Settings GetSettings(Role role, bool hasRangedOffense)
        {
            Settings s;

            switch (role)
            {
                case Role::Tank:
                    s.AllowTaunt = true;
                    s.KeepRangedDistance = false;
                    s.PreferredRangeYards = 0.0f;
                    s.DispelGroup = false;
                    s.AoeMinTargets = 2;
                    s.HealThresholdPct = 80;
                    break;

                case Role::Healer:
                    s.AllowTaunt = false;
                    s.KeepRangedDistance = true;
                    s.PreferredRangeYards = 28.0f;
                    s.DispelGroup = true;
                    s.AoeMinTargets = 4;
                    s.HealThresholdPct = 88;
                    s.SelfHealPct = 65;
                    s.MasterHealPct = 75;
                    break;

                case Role::Dps:
                default:
                    s.AllowTaunt = false;
                    s.KeepRangedDistance = hasRangedOffense;
                    s.PreferredRangeYards = hasRangedOffense ? 25.0f : 0.0f;
                    s.DispelGroup = false;
                    s.AoeMinTargets = 3;
                    s.HealThresholdPct = 85;
                    break;
            }

            return s;
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_POLICY_COMBATROLEPOLICY_H