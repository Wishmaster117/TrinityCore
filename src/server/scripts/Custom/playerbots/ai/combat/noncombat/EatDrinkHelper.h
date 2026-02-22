#ifndef PLAYERBOTS_AI_COMBAT_NONCOMBAT_EATDRINKHELPER_H
#define PLAYERBOTS_AI_COMBAT_NONCOMBAT_EATDRINKHELPER_H

#include "Config.h"
#include "Timer.h"

#include "Player.h"
#include "Unit.h"
#include "Bag.h"
#include "Item.h"
#include "SpellDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellAuraDefines.h"
#include "SharedDefines.h"
#include "ItemTemplate.h"

#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

#include <array>
#include <cstdint>

namespace Playerbots::AI::Combat::NonCombat
{
    // Very small local cooldown to avoid spamming inventory scan / item use every tick when conditions aren't met.
    struct EatDrinkCooldownSlot
    {
        uint32 BotGuidLow = 0;
        uint32 UntilMs = 0;
    };

    static constexpr uint32 kCooldownSlots = 128;

    inline std::array<EatDrinkCooldownSlot, kCooldownSlots>& CooldownRing()
    {
        static std::array<EatDrinkCooldownSlot, kCooldownSlots> s_ring{};
        return s_ring;
    }

    inline uint32& CooldownCursor()
    {
        static uint32 s_cursor = 0;
        return s_cursor;
    }

    inline bool IsOnCooldown(Player* bot, uint32 nowMs)
    {
        if (!bot)
            return true;

        uint32 low = bot->GetGUID().GetCounter();
        for (EatDrinkCooldownSlot& s : CooldownRing())
        {
            if (!s.BotGuidLow)
                continue;

            if (s.UntilMs <= nowMs)
            {
                s.BotGuidLow = 0;
                s.UntilMs = 0;
                continue;
            }

            if (s.BotGuidLow == low)
                return true;
        }
        return false;
    }

    inline void SetCooldown(Player* bot, uint32 nowMs, uint32 delayMs)
    {
        if (!bot)
            return;

        uint32 low = bot->GetGUID().GetCounter();
        // Update existing slot first.
        for (EatDrinkCooldownSlot& s : CooldownRing())
        {
            if (s.BotGuidLow == low)
            {
                s.UntilMs = nowMs + delayMs;
                return;
            }
        }

        EatDrinkCooldownSlot& s = CooldownRing()[CooldownCursor() % kCooldownSlots];
        CooldownCursor() = (CooldownCursor() + 1) % kCooldownSlots;
        s.BotGuidLow = low;
        s.UntilMs = nowMs + delayMs;
    }

    inline bool HasNearbyHostile(Player* bot, float radius)
    {
        if (!bot)
            return true;

        // Early exit scan (we just need to know if at least one hostile is close).
        std::list<Unit*> targets;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(bot, bot, radius);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, targets, check);
        Cell::VisitAllObjects(bot, searcher, radius);

        for (Unit* u : targets)
        {
            if (!u || !u->IsAlive())
                continue;

            if (!bot->IsValidAttackTarget(u))
                continue;

            // Exclude player-controlled units (pets/totems/guardians), but keep players.
            if (!u->IsPlayer() && u->IsControlledByPlayer())
                continue;

            return true;
        }

        return false;
    }

    inline bool IsCurrentlyEating(Player* bot)
    {
        return bot && bot->HasAuraType(SPELL_AURA_MOD_REGEN);
    }

    inline bool IsCurrentlyDrinking(Player* bot)
    {
        return bot && bot->HasAuraType(SPELL_AURA_MOD_POWER_REGEN);
    }

    inline bool CanAttemptEatDrink(Player* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        // Must be safely out of combat.
        if (bot->IsInCombat() || bot->GetVictim())
            return false;

        // Don't interrupt current cast/move – humans stop first.
        if (bot->IsNonMeleeSpellCast(false) || bot->isMoving())
            return false;

        // Avoid weird states.
        // Unit::IsOnVehicle(Unit const*) in this core, so use GetVehicle() as a generic check.
        if (bot->IsMounted() || bot->IsInFlight() || bot->GetVehicle())
            return false;

        return true;
    }

    inline bool IsFoodOrDrinkUseSpell(Player* bot, uint32 spellId, bool wantDrink)
    {
        if (!bot || !spellId || !bot->GetMap())
            return false;

        Difficulty diff = Difficulty(bot->GetMap()->GetDifficultyID());
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, diff);
        if (!info || info->IsPassive())
            return false;

        // SpellCategory is stable in TC:
        // FOOD = 11, DRINK = 59 (SharedDefines.h)
        uint32 cat = info->CategoryId;
        if (wantDrink)
            return cat == SPELL_CATEGORY_DRINK;
        return cat == SPELL_CATEGORY_FOOD;
    }

    inline uint32 GetFirstUseSpellId(ItemTemplate const* proto)
    {
        if (!proto)
            return 0;

        // Modern TC: item use spells are described by ItemEffectEntry in proto->Effects.
        // Pick the first ON_USE effect with a valid SpellID.
        for (ItemEffectEntry const* eff : proto->Effects)
        {
            if (!eff)
                continue;

            if (eff->TriggerType != ITEM_SPELLTRIGGER_ON_USE)
                continue;

            if (eff->SpellID > 0)
                return uint32(eff->SpellID);
        }

        return 0;
    }

    inline Item* FindBestFoodOrDrink(Player* bot, bool wantDrink)
    {
        if (!bot)
            return nullptr;

        Item* best = nullptr;
        uint32 bestIlvl = 0;

        auto consider = [&](Item* it)
        {
            if (!it)
                return;

            ItemTemplate const* proto = it->GetTemplate();
            if (!proto)
                return;

            // Avoid unusable level requirements.
            // In this core, it's exposed via GetBaseRequiredLevel().
            if (proto->GetBaseRequiredLevel() > 0 && bot->GetLevel() < uint32(proto->GetBaseRequiredLevel()))
                return;

            // We only accept items that have a use spell matching FOOD/DRINK category.
            uint32 useSpell = GetFirstUseSpellId(proto);
            if (!useSpell)
                return;

            if (!IsFoodOrDrinkUseSpell(bot, useSpell, wantDrink))
                return;

            uint32 ilvl = std::max<uint32>(1u, proto->GetBaseItemLevel());
            if (!best || ilvl > bestIlvl)
            {
                best = it;
                bestIlvl = ilvl;
            }
        };

        // Backpack slots.
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            consider(bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));

        // Bags.
        for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        {
            Item* bagItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bagSlot);
            Bag* bag = bagItem ? bagItem->ToBag() : nullptr;
            if (!bag)
                continue;

            for (uint32 i = 0; i < bag->GetBagSize(); ++i)
                consider(bag->GetItemByPos(i));
        }

        return best;
    }

    inline bool TryUseFoodOrDrink(Player* bot, Item* item)
    {
        if (!bot || !item)
            return false;

        // Use proper item use spell path (consumption/cooldowns handled by core).
        SpellCastTargets targets;
        targets.SetUnitTarget(bot);

        bot->CastItemUseSpell(item, targets, ObjectGuid::Empty, std::array<int32, 3>{ { 0, 0, 0 } });
        return true;
    }

    // Main entry point used by NonCombat rotations.
    // Returns true if we attempted to start eating/drinking (i.e., we used an item).
    inline bool TryEatDrink(Player* bot)
    {
        if (!CanAttemptEatDrink(bot))
            return false;

        uint32 nowMs = getMSTime();
        if (IsOnCooldown(bot, nowMs))
            return false;

        // “Human-like” safety: don’t sit to eat/drink with enemies close.
        float safeRadius = sConfigMgr->GetFloatDefault("Playerbots.NonCombat.EatDrink.SafeEnemyRadius", 30.0f);
        if (safeRadius < 5.0f) safeRadius = 5.0f;
        if (safeRadius > 80.0f) safeRadius = 80.0f;
        if (HasNearbyHostile(bot, safeRadius))
        {
            SetCooldown(bot, nowMs, 1500);
            return false;
        }

        uint32 hpPct = uint32(bot->GetHealthPct());
        bool isManaUser = (bot->GetPowerType() == POWER_MANA);
        uint32 manaPct = isManaUser ? uint32(bot->GetPowerPct(POWER_MANA)) : 100;

        uint32 eatHp = uint32(sConfigMgr->GetIntDefault("Playerbots.NonCombat.EatHpPct", 65));
        uint32 drinkMana = uint32(sConfigMgr->GetIntDefault("Playerbots.NonCombat.DrinkManaPct", 35));
        if (eatHp < 1) eatHp = 1;
        if (eatHp > 99) eatHp = 99;
        if (drinkMana < 1) drinkMana = 1;
        if (drinkMana > 99) drinkMana = 99;

        bool needEat = (hpPct < eatHp) && !IsCurrentlyEating(bot);
        bool needDrink = isManaUser && (manaPct < drinkMana) && !IsCurrentlyDrinking(bot);

        // Prefer drink first for mana users (typical human behavior) if both are needed.
        if (needDrink)
        {
            if (Item* drink = FindBestFoodOrDrink(bot, true))
            {
                bool ok = TryUseFoodOrDrink(bot, drink);
                SetCooldown(bot, nowMs, ok ? 2500 : 2500);
                return ok;
            }
        }

        if (needEat)
        {
            if (Item* food = FindBestFoodOrDrink(bot, false))
            {
                bool ok = TryUseFoodOrDrink(bot, food);
                SetCooldown(bot, nowMs, ok ? 2500 : 2500);
                return ok;
            }
        }

        // No consumables found -> backoff longer to avoid inventory scan spam.
        SetCooldown(bot, nowMs, 8000);
        return false;
    }
}

#endif // PLAYERBOTS_AI_COMBAT_NONCOMBAT_EATDRINKHELPER_H