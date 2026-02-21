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
    };
}

#endif // PLAYERBOTS_AI_COMBAT_ROTATIONS_ROTATIONCONTEXT_H