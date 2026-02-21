#ifndef PLAYERBOTS_AI_COMBAT_COMBATCONTEXT_H
#define PLAYERBOTS_AI_COMBAT_COMBATCONTEXT_H

class Player;
class Unit;

namespace Playerbots::AI::Combat
{
    struct CombatContext
    {
        Player* Bot = nullptr;
        Player* Master = nullptr;
        Unit* Victim = nullptr;

        bool IsCasting = false;
        bool IsMoving = false;
        bool BotNeedsHeal = false;
        bool MasterNeedsHeal = false;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_COMBATCONTEXT_H