#ifndef PLAYERBOTS_AI_COMBAT_SPELLCATALOG_H
#define PLAYERBOTS_AI_COMBAT_SPELLCATALOG_H

#include "Player.h"
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
        }

        void Build(Player* bot)
        {
            Clear();
            if (!bot || !bot->GetMap())
                return;

            Difficulty difficulty = Difficulty(bot->GetMap()->GetDifficultyID());

            for (auto const& [spellId, data] : bot->GetSpellMap())
            {
                if (data.state == PLAYERSPELL_REMOVED || data.disabled)
                    continue;

                SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, difficulty);
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

            auto sortOffensive = [bot, difficulty](uint32 a, uint32 b)
            {
                SpellInfo const* ia = sSpellMgr->GetSpellInfo(a, difficulty);
                SpellInfo const* ib = sSpellMgr->GetSpellInfo(b, difficulty);
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

            auto sortHeals = [bot, difficulty](uint32 a, uint32 b)
            {
                SpellInfo const* ia = sSpellMgr->GetSpellInfo(a, difficulty);
                SpellInfo const* ib = sSpellMgr->GetSpellInfo(b, difficulty);
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
            return !bot || _knownSpellCount != uint32(bot->GetSpellMap().size());
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
    };
}

#endif // PLAYERBOTS_AI_COMBAT_SPELLCATALOG_H