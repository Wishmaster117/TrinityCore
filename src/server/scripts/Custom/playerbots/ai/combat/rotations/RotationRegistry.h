#ifndef PLAYERBOTS_AI_COMBAT_ROTATIONS_ROTATIONREGISTRY_H
#define PLAYERBOTS_AI_COMBAT_ROTATIONS_ROTATIONREGISTRY_H

#include "Player.h"

namespace Playerbots::AI::Combat::Rotations
{
    class ICombatRotation;
    class INonCombatRotation;

    // Note:
    // We intentionally keep this registry header-only and dependency-light.
    // It returns singleton instances for each class (and later spec).
    class RotationRegistry final
    {
    public:

        static ICombatRotation* GetCombatRotation(Player* bot);
        static INonCombatRotation* GetNonCombatRotation(Player* bot);

    private:
        static uint8 GetBotClassId(Player* bot) { return bot ? bot->GetClass() : 0; }
        static uint32 GetBotSpecKey(Player* /*bot*/) { return 0; }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_ROTATIONS_ROTATIONREGISTRY_H