#ifndef PLAYERBOTS_AI_COMBAT_SPELLCATALOG_H
#define PLAYERBOTS_AI_COMBAT_SPELLCATALOG_H

#include "Map.h"
#include "Player.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "playerbots/ai/combat/SpellClassifier.h"

namespace Playerbots::AI::Combat
{
    class SpellCatalog final
    {
    public:
        void Clear()
        {
            _offensive.clear();
            _melee.clear();
            _aoe.clear();
            _heals.clear();
            _buffs.clear();
            _dispels.clear();
            _interrupts.clear();
            _cc.clear();
            _taunts.clear();
            _knownSpellCount = 0;
            _knownUsableSpellCount = 0;
            _difficulty = Difficulty(0);
        }

        void Build(Player* bot)
        {
            Clear();
            if (!bot || !bot->GetMap())
                return;

            _difficulty = Difficulty(bot->GetMap()->GetDifficultyID());
            _knownUsableSpellCount = 0;

            for (auto const& [spellId, data] : bot->GetSpellMap())
            {
                if (data.state == PLAYERSPELL_REMOVED || data.disabled)
                    continue;

                ++_knownUsableSpellCount;

                SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, _difficulty);
                if (!info)
                    continue;

                if (SpellClassifier::IsUsableOffensiveSpell(info, bot))
                    _offensive.push_back(spellId);

                if (SpellClassifier::IsUsableMeleeAbility(info, bot))
                    _melee.push_back(spellId);

                if (SpellClassifier::IsUsableAoeOffensiveSpell(info, bot))
                    _aoe.push_back(spellId);


                if (SpellClassifier::IsUsableHealSpell(info, bot))
                    _heals.push_back(spellId);

                if (SpellClassifier::IsUsableBuffSpell(info, bot))
                    _buffs.push_back(spellId);

                if (SpellClassifier::IsUsableDispelSpell(info))
                    _dispels.push_back(spellId);

                if (SpellClassifier::IsUsableInterruptSpell(info))
                    _interrupts.push_back(spellId);

                if (SpellClassifier::IsUsableCrowdControlSpell(info))
                    _cc.push_back(spellId);


                if (SpellClassifier::IsUsableTauntSpell(info))
                    _taunts.push_back(spellId);
            }

            _knownSpellCount = uint32(bot->GetSpellMap().size());

            auto sortOffensive = [bot, this](uint32 a, uint32 b)
            {
                SpellInfo const* ia = sSpellMgr->GetSpellInfo(a, _difficulty);
                SpellInfo const* ib = sSpellMgr->GetSpellInfo(b, _difficulty);
                if (!ia || !ib)
                    return a < b;

                float ra = ia->GetMaxRange(false, bot, nullptr);
                float rb = ib->GetMaxRange(false, bot, nullptr);
                if (ra != rb)
                    return ra > rb;

                uint32 ca = ia->CalcCastTime();
                uint32 cb = ib->CalcCastTime();
                if (ca != cb)
                    return ca < cb;

                return a < b;
            };

            auto sortHeals = [bot, this](uint32 a, uint32 b)
            {
                SpellInfo const* ia = sSpellMgr->GetSpellInfo(a, _difficulty);
                SpellInfo const* ib = sSpellMgr->GetSpellInfo(b, _difficulty);
                if (!ia || !ib)
                    return a < b;

                uint32 ca = ia->CalcCastTime();
                uint32 cb = ib->CalcCastTime();
                if (ca != cb)
                    return ca < cb;

                float ra = ia->GetMaxRange(true, bot, nullptr);
                float rb = ib->GetMaxRange(true, bot, nullptr);
                if (ra != rb)
                    return ra > rb;

                return a < b;
            };

            std::sort(_offensive.begin(), _offensive.end(), sortOffensive);
            // Melee abilities: prefer "weapon-like" / faster cast (all 0), then stable id.
            std::sort(_melee.begin(), _melee.end());
            std::sort(_aoe.begin(), _aoe.end(), sortOffensive);
            std::sort(_heals.begin(), _heals.end(), sortHeals);
            std::sort(_buffs.begin(), _buffs.end());
            std::sort(_dispels.begin(), _dispels.end());
            std::sort(_interrupts.begin(), _interrupts.end());
            std::sort(_cc.begin(), _cc.end());
            std::sort(_taunts.begin(), _taunts.end());
        }

        bool NeedsRebuild(Player* bot) const
        {
            if (!bot)
                return true;

            // Size change = definitely rebuild.
            if (_knownSpellCount != uint32(bot->GetSpellMap().size()))
                return true;

            // Also rebuild if usability changed without size changing (disabled/removed toggles).
            uint32 usable = 0;
            for (auto const& [spellId, data] : bot->GetSpellMap())
            {
                if (data.state == PLAYERSPELL_REMOVED || data.disabled)
                    continue;
                ++usable;
            }

            return usable != _knownUsableSpellCount;
        }

        std::vector<uint32> const& GetOffensive() const { return _offensive; }
        std::vector<uint32> const& GetMelee() const { return _melee; }
        std::vector<uint32> const& GetAoe() const { return _aoe; }
        std::vector<uint32> const& GetHeals() const { return _heals; }
        std::vector<uint32> const& GetBuffs() const { return _buffs; }
        std::vector<uint32> const& GetDispels() const { return _dispels; }
        std::vector<uint32> const& GetInterrupts() const { return _interrupts; }
        std::vector<uint32> const& GetCrowdControls() const { return _cc; }
        std::vector<uint32> const& GetTaunts() const { return _taunts; }

        bool HasOffensive() const { return !_offensive.empty(); }
        bool HasMelee() const { return !_melee.empty(); }
        bool HasAoe() const { return !_aoe.empty(); }
        bool HasHeals() const { return !_heals.empty(); }
        bool HasBuffs() const { return !_buffs.empty(); }
        bool HasDispels() const { return !_dispels.empty(); }
        bool HasInterrupts() const { return !_interrupts.empty(); }
        bool HasCrowdControls() const { return !_cc.empty(); }
        bool HasTaunts() const { return !_taunts.empty(); }

        // "Caster-ish": has any ranged offensive spell candidates.
        bool HasRangedOffense() const { return HasOffensive(); }

        uint32 OffensiveCount() const { return uint32(_offensive.size()); }
        uint32 HealCount() const { return uint32(_heals.size()); }

        // Shared helper: aura presence across spell chain (centralized here to avoid duplication).
        bool TargetHasAuraFromSpellChain(Unit* target, uint32 spellId) const
        {
            if (!target || !spellId)
                return false;

            uint32 first = sSpellMgr->GetFirstSpellInChain(spellId);
            if (!first)
                first = spellId;

            for (uint32 id = first; id; id = sSpellMgr->GetNextSpellInChain(id))
            {
                if (target->HasAura(id))
                    return true;
            }
            return false;
        }

        // 3.4: Spellbook-driven non-combat helper:
        // Pick a missing self-only buff (armor-like) without hardcoded IDs.
        //
        // Heuristic:
        // - must be in the Buff bucket
        // - must be self-only (max range <= 0)
        // - must apply an aura (already ensured by buff classification)
        // - must NOT already be present on the bot (spell chain aware)
        uint32 BestMissingSelfOnlyBuff(Player* bot) const
        {
            if (!bot || _buffs.empty() || !_difficulty)
                return 0;

            for (uint32 id : _buffs)
            {
                SpellInfo const* info = sSpellMgr->GetSpellInfo(id, _difficulty);
                if (!info)
                    continue;

                // Ensure it is a usable buff (positive, aura apply, not heal/dispel).
                if (!SpellClassifier::IsUsableBuffSpell(info, bot))
                    continue;

                // Self-only (armor-style) buff.
                float range = info->GetMaxRange(true, bot, nullptr);
                if (range > 0.0f)
                    continue;

                if (TargetHasAuraFromSpellChain(bot, id))
                    continue;

                return id;
            }

            return 0;
        }

        // 3.4 FINAL: spellbook-driven non-combat helper:
        // Pick a missing "group-style" buff (range > 0) for a specific target (master).
        // No hardcoded IDs; conservative gating is done by caller (non-combat rotation).
        uint32 BestMissingGroupBuffForTarget(Player* bot, Unit* target) const
        {
            if (!bot || !target || _buffs.empty() || !_difficulty)
                return 0;

            for (uint32 id : _buffs)
            {
                SpellInfo const* info = sSpellMgr->GetSpellInfo(id, _difficulty);
                if (!info)
                    continue;

                if (!SpellClassifier::IsUsableBuffSpell(info, bot))
                    continue;

                // Group/targetable buff: range > 0 (self-only armors already handled elsewhere).
                float range = info->GetMaxRange(true, bot, nullptr);
                if (range <= 0.0f)
                    continue;

                // Avoid rebuffing if already present (spell chain aware).
                if (TargetHasAuraFromSpellChain(target, id))
                    continue;

                return id;
            }

            return 0;
        }

        // Spellbook-driven: best safe instant nuke (direct damage) by school.
        uint32 BestInstantNukeBySchool(Player* bot, SpellSchoolMask schoolMask) const
        {
            if (!bot || _offensive.empty())
                return 0;

            for (uint32 id : _offensive)
            {
                SpellInfo const* info = sSpellMgr->GetSpellInfo(id, _difficulty);
                if (SpellClassifier::IsSafeRangedSingleTargetInstantNuke(info, bot, schoolMask))
                    return id;
            }
            return 0;
        }

        // Spellbook-driven: best safe single-target DoT by school.
        // We prefer instant DoTs first, then cast-time DoTs.
        uint32 BestSingleTargetDotBySchool(Player* bot, SpellSchoolMask schoolMask) const
        {
            if (!bot || _offensive.empty())
                return 0;

            for (uint32 id : _offensive)
            {
                SpellInfo const* info = sSpellMgr->GetSpellInfo(id, _difficulty);
                if (SpellClassifier::IsSafeRangedSingleTargetDot(info, bot, schoolMask) && info->CalcCastTime() == 0)
                    return id;
            }

            for (uint32 id : _offensive)
            {
                SpellInfo const* info = sSpellMgr->GetSpellInfo(id, _difficulty);
                if (SpellClassifier::IsSafeRangedSingleTargetDot(info, bot, schoolMask) && info->CalcCastTime() != 0)
                    return id;
            }
            return 0;
        }

        // Spellbook-driven: best safe AOE spell by school (or any school when mask == NONE).
        uint32 BestAoeBySchool(Player* bot, SpellSchoolMask schoolMask) const
        {
            if (!bot || _aoe.empty())
                return 0;

            for (uint32 id : _aoe)
            {
                SpellInfo const* info = sSpellMgr->GetSpellInfo(id, _difficulty);
                if (SpellClassifier::IsSafeAoeOffensiveSpell(info, bot, schoolMask))
                    return id;
            }
            return 0;
        }

        // Spellbook-driven selection helpers (MVP, no hardcoded SpellIDs).
        // Picks a ranged, single-target direct-damage spell matching the requested school.
        // Heuristic:
        //  - pass 1: prefer cast-time nukes (Fireball/Frostbolt/Arcane Blast-like)
        //  - pass 2: fallback to instant direct-damage (proc/finisher-like)
        uint32 BestSingleTargetNukeBySchool(Player* bot, SpellSchoolMask schoolMask) const
        {
            if (!bot || _offensive.empty())
                return 0;

            auto safeInstant = [this, bot, schoolMask](uint32 spellId) -> bool
            {
                SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, _difficulty);
                if (!info)
                    return false;

                // Conservative instant fallback: still avoid cooldowns and channels.
                if (!SpellClassifier::IsSafeRangedSingleTargetDirectDamage(info, bot, schoolMask))
                    return false;

                if (info->IsChanneled())
                    return false;

                if (info->CalcCastTime() != 0)
                    return false;

                // Avoid picking burst/utility instants as fillers.
                if (SpellClassifier::HasAnyCooldown(info))
                    return false;

                return true;
            };

            auto safeOffensiveCastTime = [this, bot, schoolMask](uint32 spellId) -> bool
            {
                SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, _difficulty);
                if (!info)
                    return false;

                if (!SpellClassifier::IsSafeRangedSingleTargetOffensive(info, bot, schoolMask))
                    return false;

                // Prefer cast-time fillers first in this pass.
                return (info->CalcCastTime() != 0);
            };

            auto safeOffensiveInstant = [this, bot, schoolMask](uint32 spellId) -> bool
            {
                SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, _difficulty);
                if (!info)
                    return false;

                if (!SpellClassifier::IsSafeRangedSingleTargetOffensive(info, bot, schoolMask))
                    return false;

                return (info->CalcCastTime() == 0);
            };

            // Prefer "core nukes" (cast-time, no cooldown, not channeled, etc.).
            for (uint32 id : _offensive)
            {
                SpellInfo const* info = sSpellMgr->GetSpellInfo(id, _difficulty);
                if (SpellClassifier::IsLikelyCoreNuke(info, bot, schoolMask))
                    return id;
            }

            // Fallback to safe instant direct damage (still conservative).
            for (uint32 id : _offensive)
                if (safeInstant(id))
                    return id;

            // 3.3.5: last-resort filler (still safe):
            // ranged single-target offensive spell (direct damage OR DoT),
            // no AoE targeting, no channel, no cooldown.
            // Prefer cast-time first, then instant.
            for (uint32 id : _offensive)
                if (safeOffensiveCastTime(id))
                    return id;

            for (uint32 id : _offensive)
                if (safeOffensiveInstant(id))
                    return id;

            return 0;
        }

    private:
        std::vector<uint32> _offensive;
        std::vector<uint32> _melee;
        std::vector<uint32> _aoe;
        std::vector<uint32> _heals;
        std::vector<uint32> _buffs;
        std::vector<uint32> _dispels;
        std::vector<uint32> _interrupts;
        std::vector<uint32> _cc;
        std::vector<uint32> _taunts;
        uint32 _knownSpellCount = 0;
        uint32 _knownUsableSpellCount = 0;
        Difficulty _difficulty = Difficulty(0);
    };
}

#endif // PLAYERBOTS_AI_COMBAT_SPELLCATALOG_H