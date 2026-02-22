#ifndef PLAYERBOTS_AI_COMBAT_ROTATIONS_ROTATIONCONTEXT_H
#define PLAYERBOTS_AI_COMBAT_ROTATIONS_ROTATIONCONTEXT_H

class Player;
class Unit;

namespace Playerbots::AI::Combat
{
    class SpellCatalog;
}

namespace Playerbots::AI::Combat::Rotations
{
    struct RotationContext
    {
        Player* Bot = nullptr;
        Player* Master = nullptr;
        Unit* Victim = nullptr;
        SpellCatalog const* Catalog = nullptr;

        bool MasterIsTank = false;

        // AOE hinting (filled by BotCombatEngine).
        // NearbyEnemies is computed with the same radius as the engine AOE phase.
        uint32 NearbyEnemies = 0;

        // Minimum mobs required to consider AOE (from role settings).
        uint32 AoeMinTargets = 0;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_ROTATIONS_ROTATIONCONTEXT_H