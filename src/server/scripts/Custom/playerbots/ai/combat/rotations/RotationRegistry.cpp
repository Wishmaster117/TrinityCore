#include "playerbots/ai/combat/rotations/RotationRegistry.h"

#include "playerbots/ai/combat/rotations/ICombatRotation.h"
#include "playerbots/ai/combat/rotations/INonCombatRotation.h"

// Combat/Non-combat rotations per class (implementation files)
#include "playerbots/ai/combat/classes/deathknight/DeathKnightCombatRotation.h"
#include "playerbots/ai/combat/classes/deathknight/DeathKnightNonCombatRotation.h"
#include "playerbots/ai/combat/classes/demonhunter/DemonHunterCombatRotation.h"
#include "playerbots/ai/combat/classes/demonhunter/DemonHunterNonCombatRotation.h"
#include "playerbots/ai/combat/classes/druid/DruidCombatRotation.h"
#include "playerbots/ai/combat/classes/druid/DruidNonCombatRotation.h"
#include "playerbots/ai/combat/classes/evoker/EvokerCombatRotation.h"
#include "playerbots/ai/combat/classes/evoker/EvokerNonCombatRotation.h"
#include "playerbots/ai/combat/classes/hunter/HunterCombatRotation.h"
#include "playerbots/ai/combat/classes/hunter/HunterNonCombatRotation.h"
#include "playerbots/ai/combat/classes/mage/MageCombatRotation.h"
#include "playerbots/ai/combat/classes/mage/MageNonCombatRotation.h"
#include "playerbots/ai/combat/classes/monk/MonkCombatRotation.h"
#include "playerbots/ai/combat/classes/monk/MonkNonCombatRotation.h"
#include "playerbots/ai/combat/classes/paladin/PaladinCombatRotation.h"
#include "playerbots/ai/combat/classes/paladin/PaladinNonCombatRotation.h"
#include "playerbots/ai/combat/classes/priest/PriestCombatRotation.h"
#include "playerbots/ai/combat/classes/priest/PriestNonCombatRotation.h"
#include "playerbots/ai/combat/classes/rogue/RogueCombatRotation.h"
#include "playerbots/ai/combat/classes/rogue/RogueNonCombatRotation.h"
#include "playerbots/ai/combat/classes/shaman/ShamanCombatRotation.h"
#include "playerbots/ai/combat/classes/shaman/ShamanNonCombatRotation.h"
#include "playerbots/ai/combat/classes/warlock/WarlockCombatRotation.h"
#include "playerbots/ai/combat/classes/warlock/WarlockNonCombatRotation.h"
#include "playerbots/ai/combat/classes/warrior/WarriorCombatRotation.h"
#include "playerbots/ai/combat/classes/warrior/WarriorNonCombatRotation.h"

namespace Playerbots::AI::Combat::Rotations
{
    ICombatRotation* RotationRegistry::GetCombatRotation(Player* bot)
    {
        switch (GetBotClassId(bot))
        {
            case CLASS_DEATH_KNIGHT: return &DeathKnightCombatRotation::Instance();
            case CLASS_DEMON_HUNTER: return &DemonHunterCombatRotation::Instance();
            case CLASS_DRUID:        return &DruidCombatRotation::Instance();
            case CLASS_EVOKER:       return &EvokerCombatRotation::Instance();
            case CLASS_HUNTER:       return &HunterCombatRotation::Instance();
            case CLASS_MAGE:         return &MageCombatRotation::Instance();
            case CLASS_MONK:         return &MonkCombatRotation::Instance();
            case CLASS_PALADIN:      return &PaladinCombatRotation::Instance();
            case CLASS_PRIEST:       return &PriestCombatRotation::Instance();
            case CLASS_ROGUE:        return &RogueCombatRotation::Instance();
            case CLASS_SHAMAN:       return &ShamanCombatRotation::Instance();
            case CLASS_WARLOCK:      return &WarlockCombatRotation::Instance();
            case CLASS_WARRIOR:      return &WarriorCombatRotation::Instance();
            default:                 return nullptr;
        }
    }

    INonCombatRotation* RotationRegistry::GetNonCombatRotation(Player* bot)
    {
        switch (GetBotClassId(bot))
        {
            case CLASS_DEATH_KNIGHT: return &DeathKnightNonCombatRotation::Instance();
            case CLASS_DEMON_HUNTER: return &DemonHunterNonCombatRotation::Instance();
            case CLASS_DRUID:        return &DruidNonCombatRotation::Instance();
            case CLASS_EVOKER:       return &EvokerNonCombatRotation::Instance();
            case CLASS_HUNTER:       return &HunterNonCombatRotation::Instance();
            case CLASS_MAGE:         return &MageNonCombatRotation::Instance();
            case CLASS_MONK:         return &MonkNonCombatRotation::Instance();
            case CLASS_PALADIN:      return &PaladinNonCombatRotation::Instance();
            case CLASS_PRIEST:       return &PriestNonCombatRotation::Instance();
            case CLASS_ROGUE:        return &RogueNonCombatRotation::Instance();
            case CLASS_SHAMAN:       return &ShamanNonCombatRotation::Instance();
            case CLASS_WARLOCK:      return &WarlockNonCombatRotation::Instance();
            case CLASS_WARRIOR:      return &WarriorNonCombatRotation::Instance();
            default:                 return nullptr;
        }
    }
}