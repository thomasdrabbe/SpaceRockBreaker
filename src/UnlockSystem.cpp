#include "UnlockSystem.h"
#include "GameState.h"
#include "NotificationSystem.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace {

constexpr int kPhaseCount = 9;
constexpr int toIndex(UnlockPhase p) {
    return static_cast<int>(p);
}

void applyPhaseVisibility(UnlockPhase     phase,
                          GameState&      state,
                          IUnlockEffects& effects) {
    switch (phase) {
        case UnlockPhase::PLINKO_TAB:
            effects.setTabVisible(Tab::PLINKO, true);
            break;
        case UnlockPhase::SHOP_TAB:
            effects.setTabVisible(Tab::SKILL_TREE, true);
            break;
        case UnlockPhase::WEAPONS:
        case UnlockPhase::MINING:
        case UnlockPhase::AUTO_PLINKO:
        case UnlockPhase::ECONOMY:
            break;
        case UnlockPhase::CHESTS:
            effects.setTabVisible(Tab::CHESTS, true);
            state.keyAsteroidsEnabled = true;
            break;
        case UnlockPhase::PRESTIGE:
            effects.setTabVisible(Tab::PRESTIGE, true);
            break;
        default:
            break;
    }
}

} // namespace

UnlockNextHint computeUnlockNextHint(const GameState& s) {
    UnlockNextHint h;
    h.heading = "Volgende doel";

    const auto& d = s.unlockPhaseDone;
    const bool  firstBossIncoming =
        (s.nextBossMilestone == FIRST_BOSS_ZONE && s.currentLevel >= 4);

    auto fmtOre = [](double v) {
        std::ostringstream o;
        o << static_cast<long long>(v + 1e-9);
        return o.str();
    };
    auto fmtCredits = [](double v) {
        std::ostringstream o;
        o << static_cast<long long>(v + 0.5);
        return o.str();
    };

    for (int p = 1; p <= 8; ++p) {
        if (d[static_cast<std::size_t>(p)])
            continue;

        switch (p) {
        case 1:
            h.phaseName = "Plinko-tab";
            h.progressDetail =
                "Totale ore: " + fmtOre(s.totalOre) + " / 5";
            h.progress01 = static_cast<float>(
                std::clamp(s.totalOre / 5.0, 0.0, 1.0));
            return h;
        case 2:
            h.phaseName = "Skill tree";
            if (firstBossIncoming) {
                h.progressDetail =
                    "Naderende boss - skill tree opent automatisch";
                h.progress01 = 1.f;
            } else {
                h.progressDetail =
                    "Totale ore: " + fmtOre(s.totalOre) + " / 25";
                h.progress01 = static_cast<float>(
                    std::clamp(s.totalOre / 25.0, 0.0, 1.0));
            }
            return h;
        case 3:
            h.phaseName = "Weapons (zone 2)";
            if (s.currentLevel >= 2) {
                h.progressDetail = "Zone 2 bereikt";
                h.progress01     = 1.f;
            } else {
                h.progressDetail = "Warp naar zone 2 (nu zone "
                                   + std::to_string(s.currentLevel) + ")";
                h.progress01 = 0.f;
            }
            return h;
        case 4:
            h.phaseName = "Mining-upgrades";
            h.progressDetail =
                "Credits: $" + fmtCredits(s.credits) + " / $50";
            h.progress01 = static_cast<float>(
                std::clamp(s.credits / 50.0, 0.0, 1.0));
            return h;
        case 5:
            h.phaseName = "Auto-Plinko";
            h.progressDetail =
                "Credits: $" + fmtCredits(s.credits) + " / $100";
            h.progress01 = static_cast<float>(
                std::clamp(s.credits / 100.0, 0.0, 1.0));
            return h;
        case 6:
            h.phaseName = "Chests & sleutels";
            if (s.currentLevel >= 3) {
                h.progressDetail = "Zone 3+ - chest-tab en sleutels";
                h.progress01     = 1.f;
            } else {
                h.progressDetail = "Warp naar zone 3 (nu zone "
                                   + std::to_string(s.currentLevel) + ")";
                h.progress01 = static_cast<float>(std::clamp(
                    static_cast<double>(s.currentLevel - 1) / 2.0, 0.0, 0.99));
            }
            return h;
        case 7:
            h.phaseName = "Economy & ore tiers";
            if (s.currentLevel >= 3) {
                h.progressDetail = "Zone 3+ - volledige skill tree";
                h.progress01     = 1.f;
            } else {
                h.progressDetail = "Warp naar zone 3 (nu zone "
                                   + std::to_string(s.currentLevel) + ")";
                h.progress01 = static_cast<float>(std::clamp(
                    static_cast<double>(s.currentLevel - 1) / 2.0, 0.0, 0.99));
            }
            return h;
        case 8:
            h.phaseName = "Prestige";
            if (s.currentLevel >= 5) {
                h.progressDetail = "Zone 5+ - prestige-tab";
                h.progress01     = 1.f;
            } else {
                h.progressDetail = "Warp naar zone 5 (nu zone "
                                   + std::to_string(s.currentLevel) + ")";
                h.progress01 = static_cast<float>(std::clamp(
                    static_cast<double>(s.currentLevel - 1) / 4.0, 0.0, 0.99));
            }
            return h;
        default:
            break;
        }
    }

    h.allPhasesComplete = true;
    h.phaseName         = "Alles vrijgespeeld";
    h.progressDetail    = "Verken prestige en eindgame-upgrades.";
    h.progress01        = 1.f;
    return h;
}

void UnlockSystem::update(GameState&           state,
                          NotificationSystem&  notifications,
                          IUnlockEffects&      effects) {
    const bool firstBossIncoming =
        (state.nextBossMilestone == FIRST_BOSS_ZONE && state.currentLevel >= 4);

    for (int p = 1; p < kPhaseCount; ++p) {
        if (state.unlockPhaseDone[static_cast<std::size_t>(p)])
            applyPhaseVisibility(static_cast<UnlockPhase>(p), state, effects);
    }

    auto markDone = [&](UnlockPhase p) {
        state.unlockPhaseDone[static_cast<std::size_t>(p)] = true;
        applyPhaseVisibility(p, state, effects);
    };

    if (!state.unlockPhaseDone[toIndex(UnlockPhase::PLINKO_TAB)]
        && state.totalOre >= 5.0) {
        if (state.upgradeLevels[static_cast<int>(UpgradeID::PLINKO_BALLS)] == 0)
            state.upgradeLevels[static_cast<int>(UpgradeID::PLINKO_BALLS)] = 15;
        notifications.push(
            "Plinko unlocked! Je eerste 15 balls zijn gratis.",
            sf::Color(255, 230, 120),
            4.f,
            -1);
        markDone(UnlockPhase::PLINKO_TAB);
    }

    if (!state.unlockPhaseDone[toIndex(UnlockPhase::SHOP_TAB)]
        && (state.totalOre >= 25.0 || firstBossIncoming)) {
        notifications.push(
            "Skill tree unlocked! Koop Warp Drive om naar de volgende zone te gaan.",
            sf::Color(120, 200, 255),
            4.f,
            -1);
        markDone(UnlockPhase::SHOP_TAB);
    }

    if (!state.unlockPhaseDone[toIndex(UnlockPhase::WEAPONS)]
        && (state.currentLevel >= 2 || firstBossIncoming)) {
        notifications.push(
            "Zone 2 bereikt! Weapon-upgrades in de skill tree.",
            sf::Color(255, 180, 140),
            4.f,
            static_cast<int>(Tab::SKILL_TREE));
        markDone(UnlockPhase::WEAPONS);
        effects.setTabBadge(Tab::SKILL_TREE);
    }

    if (!state.unlockPhaseDone[toIndex(UnlockPhase::MINING)]
        && state.credits >= 50.0) {
        notifications.push(
            "Mining upgrades beschikbaar in de skill tree!",
            sf::Color(160, 220, 255),
            4.f,
            -1);
        markDone(UnlockPhase::MINING);
    }

    if (!state.unlockPhaseDone[toIndex(UnlockPhase::AUTO_PLINKO)]
        && state.credits >= 100.0) {
        notifications.push(
            "Auto-Plinko beschikbaar! Open de skill tree (tab 3) en koop Auto-Plinko.",
            sf::Color(255, 200, 120),
            4.f,
            static_cast<int>(Tab::SKILL_TREE));
        markDone(UnlockPhase::AUTO_PLINKO);
        effects.setTabBadge(Tab::SKILL_TREE);
    }

    if (!state.unlockPhaseDone[toIndex(UnlockPhase::CHESTS)]
        && state.currentLevel >= 3) {
        notifications.push(
            "Chests unlocked! Schiet Key Asteroids neer voor sleutels.",
            sf::Color(255, 220, 160),
            4.f,
            -1);
        markDone(UnlockPhase::CHESTS);
        state.keyAsteroidsEnabled = true;
    }

    if (!state.unlockPhaseDone[toIndex(UnlockPhase::ECONOMY)]
        && state.currentLevel >= 3) {
        notifications.push(
            "Economy- en ore tier-upgrades beschikbaar in de skill tree!",
            sf::Color(220, 180, 255),
            4.f,
            -1);
        markDone(UnlockPhase::ECONOMY);
    }

    if (!state.unlockPhaseDone[toIndex(UnlockPhase::PRESTIGE)]
        && state.currentLevel >= 5) {
        notifications.push(
            "Prestige beschikbaar! Reset voor permanente crystal bonussen.",
            sf::Color(200, 160, 255),
            4.f,
            -1);
        markDone(UnlockPhase::PRESTIGE);
    }

    if (state.currentLevel >= FIRST_BOSS_ZONE
        && state.nextBossMilestone == FIRST_BOSS_ZONE
        && state.levelOf(UpgradeID::GUN_DAMAGE) == 0) {
        state.upgradeLevels[static_cast<int>(UpgradeID::GUN_DAMAGE)] = 1;
        notifications.push(
            "Gratis Gun Damage (lv1) voor je eerste boss!",
            sf::Color(255, 180, 140),
            4.5f,
            static_cast<int>(Tab::SKILL_TREE));
        effects.setTabBadge(Tab::SKILL_TREE);
    }

    effects.setKeyAsteroidsEnabled(state.keyAsteroidsEnabled);
}
