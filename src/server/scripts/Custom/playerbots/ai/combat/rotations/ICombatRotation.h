#ifndef PLAYERBOTS_AI_COMBAT_ROTATIONS_ICOMBATROTATION_H
#define PLAYERBOTS_AI_COMBAT_ROTATIONS_ICOMBATROTATION_H

#include <cstdint>

namespace Playerbots::AI::Combat::Rotations
{
    struct RotationContext;

    // Combat rotation returns:
    // - spellId to cast (base id, engine will resolve highest known rank)
    // - 0 means "no opinion" => engine fallback (generic spellbook)
    class ICombatRotation
    {
    public:
        virtual ~ICombatRotation() = default;
        virtual char const* Name() const = 0;
        virtual uint32 SelectSpell(RotationContext const& ctx) = 0;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_ROTATIONS_ICOMBATROTATION_H