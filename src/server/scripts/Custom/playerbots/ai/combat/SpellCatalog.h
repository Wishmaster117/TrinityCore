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
            _heals.clear();
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

                if (SpellClassifier::IsUsableHealSpell(info, bot))
                    _heals.push_back(spellId);

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
            std::sort(_heals.begin(), _heals.end(), sortHeals);
            std::sort(_taunts.begin(), _taunts.end());
        }

        bool NeedsRebuild(Player* bot) const
        {
            return !bot || _knownSpellCount != uint32(bot->GetSpellMap().size());
        }

        std::vector<uint32> const& GetOffensive() const { return _offensive; }
        std::vector<uint32> const& GetMelee() const { return _melee; }
       std::vector<uint32> const& GetHeals() const { return _heals; }
        std::vector<uint32> const& GetTaunts() const { return _taunts; }

        bool HasOffensive() const { return !_offensive.empty(); }
        bool HasMelee() const { return !_melee.empty(); }
       bool HasHeals() const { return !_heals.empty(); }
        bool HasTaunts() const { return !_taunts.empty(); }

        // "Caster-ish": has any ranged offensive spell candidates.
        bool HasRangedOffense() const { return HasOffensive(); }

    private:
        std::vector<uint32> _offensive;
        std::vector<uint32> _melee;
        std::vector<uint32> _heals;
        std::vector<uint32> _taunts;
        uint32 _knownSpellCount = 0;
    };
}

#endif // PLAYERBOTS_AI_COMBAT_SPELLCATALOG_H