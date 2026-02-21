#ifndef PLAYERBOTS_AI_COMBAT_SPELLCLASSIFIER_H
#define PLAYERBOTS_AI_COMBAT_SPELLCLASSIFIER_H

#include "Player.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
#include "SpellDefines.h"

namespace Playerbots::AI::Combat
{
    class SpellClassifier final
    {
    public:
        static bool HasDirectDamageEffect(SpellInfo const* spellInfo)
        {
            if (!spellInfo)
                return false;

            for (SpellEffectInfo const& eff : spellInfo->GetEffects())
            {
                switch (eff.Effect)
                {
                    case SPELL_EFFECT_SCHOOL_DAMAGE:
                    case SPELL_EFFECT_WEAPON_DAMAGE:
                    case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                    case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                    case SPELL_EFFECT_HEALTH_LEECH:
                        return true;
                    default:
                        break;
                }
            }
            return false;
        }

        static bool HasPeriodicDamageAura(SpellInfo const* spellInfo)
        {
            if (!spellInfo)
                return false;

            for (SpellEffectInfo const& eff : spellInfo->GetEffects())
            {
                if (!eff.IsAura())
                    continue;

                switch (eff.ApplyAuraName)
                {
                    case SPELL_AURA_PERIODIC_DAMAGE:
                    case SPELL_AURA_PERIODIC_LEECH:
                        return true;
                    default:
                        break;
                }
            }
            return false;
        }

        static bool HasHealingEffect(SpellInfo const* spellInfo)
        {
            if (!spellInfo)
                return false;

            for (SpellEffectInfo const& eff : spellInfo->GetEffects())
            {
                switch (eff.Effect)
                {
                    case SPELL_EFFECT_HEAL:
                    case SPELL_EFFECT_HEAL_PCT:
                    case SPELL_EFFECT_HEAL_MAX_HEALTH:
                        return true;
                    default:
                        break;
                }
            }
            return false;
        }

        static bool HasTauntEffect(SpellInfo const* spellInfo)
        {
            if (!spellInfo)
                return false;

            for (SpellEffectInfo const& eff : spellInfo->GetEffects())
            {
                // TrinityCore branches commonly use SPELL_EFFECT_ATTACK_ME for taunts.
                // (SPELL_EFFECT_TAUNT does not exist in this branch.)
                if (eff.Effect == SPELL_EFFECT_ATTACK_ME)
                    return true;
            }
            return false;
        }

        static bool IsUsableOffensiveSpell(SpellInfo const* info, Player* caster)
        {
            if (!info || !caster)
                return false;

            if (info->IsPassive() || info->IsPositive())
                return false;

            if (!HasDirectDamageEffect(info) && !HasPeriodicDamageAura(info))
                return false;

            float range = info->GetMaxRange(false, caster, nullptr);
            return range > 6.0f;
        }

        // Instant-ish melee abilities (rogue/war/etc.). We keep this conservative:
        // - range <= 6y (melee-ish)
        // - offensive with damage/dot
        // - cast time == 0 (avoid long casts in melee bucket)
        static bool IsUsableMeleeAbility(SpellInfo const* info, Player* caster)
        {
            if (!info || !caster)
                return false;

            if (info->IsPassive() || info->IsPositive())
                return false;

            if (!HasDirectDamageEffect(info) && !HasPeriodicDamageAura(info))
                return false;

            float range = info->GetMaxRange(false, caster, nullptr);
            if (range > 6.0f)
                return false;

            // Very conservative: only instant abilities here.
            if (info->CalcCastTime() != 0)
                return false;

            return true;
        }

        static bool IsUsableHealSpell(SpellInfo const* info, Player* caster)
        {
            if (!info || !caster)
                return false;

            if (info->IsPassive() || !info->IsPositive())
                return false;

            if (!HasHealingEffect(info))
                return false;

            float range = info->GetMaxRange(true, caster, nullptr);
            return range > 0.0f;
        }

        static bool IsUsableTauntSpell(SpellInfo const* info)
        {
            if (!info)
                return false;

            if (info->IsPassive())
                return false;

            // Taunts are typically non-positive and have SPELL_EFFECT_TAUNT.
            return HasTauntEffect(info);
        }
    };
}

#endif // PLAYERBOTS_AI_COMBAT_SPELLCLASSIFIER_H