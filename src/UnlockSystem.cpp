#include "UnlockSystem.h"
#include "GameState.h"
#include "NotificationSystem.h"
#include "Game.h"
#include "Shop.h"
#include "MiningScreen.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace {

constexpr int kPhaseCount = 9;

void applyPhaseVisibility(int                    phase,
                          GameState&             state,
                          Game&                  game,
                          Shop&                  shop,
                          MiningScreen& /*mining*/) {
    switch (phase) {
        case 1:
            game.setTabVisible(Tab::PLINKO, true);
            break;
        case 2:
            game.setTabVisible(Tab::SHOP, true);
            shop.setCategoryVisible(ShopCategory::MINING, true);
            // Alleen verbergen zolang latere fases die categorieën nog niet
            // vrijgegeven hebben. Anders: elke frame opnieuw `setVisible(...,
            // false)` → `ensureActiveCategoryVisible()` springt de actieve shop-tab
            // terug naar de eerste zichtbare (Weapons), ook al zijn Plinko/Economy
            // daarna weer true door fase 5/7.
            if (!state.unlockPhaseDone[5])
                shop.setCategoryVisible(ShopCategory::PLINKO, false);
            if (!state.unlockPhaseDone[7]) {
                shop.setCategoryVisible(ShopCategory::ECONOMY, false);
                shop.setCategoryVisible(ShopCategory::ORE_TIERS, false);
            }
            shop.setMiningShowsWarpOnly(true);
            break;
        case 3:
            shop.setCategoryVisible(ShopCategory::WEAPONS, true);
            break;
        case 4:
            shop.setMiningShowsWarpOnly(false);
            shop.setCategoryVisible(ShopCategory::MINING, true);
            break;
        case 5:
            // Auto-Plinko melding: Plinko-shop moet zichtbaar zijn (fase 2 zette die uit).
            shop.setCategoryVisible(ShopCategory::PLINKO, true);
            shop.setPlinkoShopAutoOnly(true);
            break;
        case 6:
            game.setTabVisible(Tab::CHESTS, true);
            state.keyAsteroidsEnabled = true;
            break;
        case 7:
            shop.setCategoryVisible(ShopCategory::PLINKO, true);
            shop.setCategoryVisible(ShopCategory::ECONOMY, true);
            shop.setCategoryVisible(ShopCategory::ORE_TIERS, true);
            shop.setPlinkoShopAutoOnly(false);
            break;
        case 8:
            game.setTabVisible(Tab::PRESTIGE, true);
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
        (s.nextBossMilestone == 3 && s.currentLevel >= 2);

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
            h.phaseName = "Shop-tab";
            if (firstBossIncoming) {
                h.progressDetail = "Naderende boss — shop opent automatisch";
                h.progress01     = 1.f;
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
                h.progressDetail = "Zone 3+ — chest-tab en sleutels";
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
                h.progressDetail = "Zone 3+ — volledige shop";
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
                h.progressDetail = "Zone 5+ — prestige-tab";
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
                          Game&                game,
                          Shop&                shop,
                          MiningScreen&        mining) {
    const bool firstBossIncoming =
        (state.nextBossMilestone == 3 && state.currentLevel >= 2);

    for (int p = 1; p < kPhaseCount; ++p) {
        if (state.unlockPhaseDone[static_cast<std::size_t>(p)])
            applyPhaseVisibility(p, state, game, shop, mining);
    }

    auto markDone = [&](int p) {
        state.unlockPhaseDone[static_cast<std::size_t>(p)] = true;
        applyPhaseVisibility(p, state, game, shop, mining);
    };

    if (!state.unlockPhaseDone[1] && state.totalOre >= 5.0) {
        if (state.upgradeLevels[static_cast<int>(UpgradeID::PLINKO_BALLS)] == 0)
            state.upgradeLevels[static_cast<int>(UpgradeID::PLINKO_BALLS)] = 15;
        notifications.push(
            "Plinko unlocked! Je eerste 15 balls zijn gratis.",
            sf::Color(255, 230, 120),
            4.f,
            -1);
        markDone(1);
    }

    if (!state.unlockPhaseDone[2]
        && (state.totalOre >= 25.0 || firstBossIncoming)) {
        notifications.push(
            "Shop unlocked! Koop Warp Drive om naar de volgende zone te gaan.",
            sf::Color(120, 200, 255),
            4.f,
            -1);
        markDone(2);
    }

    if (!state.unlockPhaseDone[3]
        && (state.currentLevel >= 2 || firstBossIncoming)) {
        notifications.push(
            "Zone 2 bereikt! Weapons upgrades beschikbaar.",
            sf::Color(255, 180, 140),
            4.f,
            -1);
        markDone(3);
        game.focusShopCategory(ShopCategory::WEAPONS);
    }

    if (!state.unlockPhaseDone[4] && state.credits >= 50.0) {
        notifications.push(
            "Mining upgrades beschikbaar!",
            sf::Color(160, 220, 255),
            4.f,
            -1);
        markDone(4);
    }

    if (!state.unlockPhaseDone[5] && state.credits >= 100.0) {
        notifications.push(
            "Auto-Plinko beschikbaar! Ga naar Shop (tab 3), categorie Plinko, "
            "en koop Auto-Plinko om tegelijk te minen en Plinko te laten lopen.",
            sf::Color(255, 200, 120),
            4.f,
            static_cast<int>(Tab::SHOP));
        markDone(5);
        game.focusShopCategory(ShopCategory::PLINKO);
    }

    if (!state.unlockPhaseDone[6] && state.currentLevel >= 3) {
        notifications.push(
            "Chests unlocked! Schiet Key Asteroids neer voor sleutels.",
            sf::Color(255, 220, 160),
            4.f,
            -1);
        markDone(6);
    }

    if (!state.unlockPhaseDone[7] && state.currentLevel >= 3) {
        notifications.push(
            "Plinko, Economy en Ore Tier upgrades beschikbaar!",
            sf::Color(220, 180, 255),
            4.f,
            -1);
        markDone(7);
    }

    if (!state.unlockPhaseDone[8] && state.currentLevel >= 5) {
        notifications.push(
            "Prestige beschikbaar! Reset voor permanente crystal bonussen.",
            sf::Color(200, 160, 255),
            4.f,
            -1);
        markDone(8);
    }

    // Eerste boss staat in zone 3: eerste Gun Damage-level gratis (geen credits).
    if (state.currentLevel >= 3 && state.nextBossMilestone == 3
        && state.levelOf(UpgradeID::GUN_DAMAGE) == 0) {
        state.upgradeLevels[static_cast<int>(UpgradeID::GUN_DAMAGE)] = 1;
        notifications.push(
            "Gratis Gun Damage (lv1) voor je eerste boss!",
            sf::Color(255, 180, 140),
            4.5f,
            static_cast<int>(Tab::SHOP));
        game.focusShopCategory(ShopCategory::WEAPONS);
    }

    // Zone 2+: Weapons hoort altijd in de shop (phase 3). Zonder dit kan phase 2
    // (elke frame WEAPONS uit) phase 3 "winnen" alleen als unlockPhaseDone[3] gezet is;
    // een haperende flag laat dan geen wapen-tab zien tijdens de eerste boss.
    shop.setCategoryVisible(ShopCategory::WEAPONS, state.currentLevel >= 2);

    mining.setKeyAsteroidsEnabled(state.keyAsteroidsEnabled);
}
