#include "GameState.h"
#include "Utils.h"
#include <fstream>
#include <cmath>
#include <algorithm>

// ═════════════════════════════════════════════════════════════
//  Static catalogs
// ═════════════════════════════════════════════════════════════
const std::array<UpgradeDef, static_cast<int>(UpgradeID::UPGRADE_COUNT)>
GameState::upgradeCatalog = {{
    // Weapons
    { "Gun Damage",        "+8 base damage per shot",         50.0,  1.55, 0 },
    { "Fire Rate",         "+0.4 shots/sec",                  80.0,  1.60, 0 },
    { "Turret Count",      "Add 1 turret",                   200.0,  2.20, 0 },
    { "Crit Chance",       "+5% crit chance",                120.0,  1.70, 0 },
    { "Crit Multiplier",   "+0.5x crit damage",              150.0,  1.75, 0 },
    { "Split Shot",        "Bullets split +1",               500.0,  2.50, 0 },
    // Mining
    { "Ore Value",         "+20% ore value",                 100.0,  1.60, 0 },
    { "Collect Radius",    "+30px collect radius",            90.0,  1.50, 0 },
    { "Ore Luck",          "+5% bonus ore drop",             110.0,  1.65, 0 },
    { "Asteroid HP",       "-10% asteroid HP",               130.0,  1.70, 0 },
    // Plinko
    { "Plinko Rows",       "Add 1 row to Plinko",            300.0,  2.00, 8  },
    { "Plinko Multiplier", "+10% slot multipliers",          200.0,  1.80, 0  },
    { "Plinko Balls",      "+1 max ball at once",            180.0,  1.85, 0  },
    { "Plinko Luck",       "+5% high-slot luck",             160.0,  1.70, 0  },
    // Economy
    { "Credit Multiplier", "+25% all credits",               250.0,  1.90, 0 },
    { "Bulk Processor",    "Convert more ore per drop",      400.0,  2.10, 0 },
    { "Auto-Plinko",       "Auto-drop balls",                750.0,  2.30, 0 },
}};

const std::array<PrestigeUpgradeDef,
    static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT)>
GameState::prestigeCatalog = {{
    { "Crystal Damage",  "+15% gun damage permanently",  1.0, 1.80, 0 },
    { "Crystal Mining",  "+15% ore value permanently",   1.0, 1.80, 0 },
    { "Crystal Economy", "+15% all credits permanently", 1.0, 1.80, 0 },
    { "Crystal Plinko",  "+15% plinko multipliers",      1.0, 1.80, 0 },
    { "Deep Retention",  "Keep 2 extra upgrades/level",  3.0, 2.50, 0 },
}};

// ═════════════════════════════════════════════════════════════
//  Crystal bonuses
// ═════════════════════════════════════════════════════════════
float GameState::_crystalDamageBonus() const {
    return 1.f + prestigeLevels[static_cast<int>(
        PrestigeUpgradeID::CRYSTAL_DAMAGE)] * 0.15f;
}
float GameState::_crystalMiningBonus() const {
    return 1.f + prestigeLevels[static_cast<int>(
        PrestigeUpgradeID::CRYSTAL_MINING)] * 0.15f;
}
float GameState::_crystalEconomyBonus() const {
    return 1.f + prestigeLevels[static_cast<int>(
        PrestigeUpgradeID::CRYSTAL_ECONOMY)] * 0.15f;
}
float GameState::_crystalPlinkoBonus() const {
    return 1.f + prestigeLevels[static_cast<int>(
        PrestigeUpgradeID::CRYSTAL_PLINKO)] * 0.15f;
}

float GameState::crystalAmp() const {
    return _crystalDamageBonus() * _crystalMiningBonus()
         * _crystalEconomyBonus() * _crystalPlinkoBonus();
}

// ═════════════════════════════════════════════════════════════
//  Computed stats
// ═════════════════════════════════════════════════════════════
float GameState::gunDamage() const {
    return (10.f + levelOf(UpgradeID::GUN_DAMAGE) * 8.f)
           * _crystalDamageBonus();
}

float GameState::fireRatePerSec() const {
    return 1.5f + levelOf(UpgradeID::FIRE_RATE) * 0.4f;
}

int GameState::turretCount() const {
    return 1 + levelOf(UpgradeID::TURRET_COUNT);
}

float GameState::critChance() const {
    return clamp(levelOf(UpgradeID::CRIT_CHANCE) * 0.05f, 0.f, 0.95f);
}

float GameState::critMult() const {
    return 2.f + levelOf(UpgradeID::CRIT_MULT) * 0.5f;
}

int GameState::splitShot() const {
    return 1 + levelOf(UpgradeID::SPLIT_SHOT);
}

float GameState::oreValueMult() const {
    return (1.f + levelOf(UpgradeID::ORE_VALUE) * 0.2f)
           * _crystalMiningBonus();
}

float GameState::autoCollectRadius() const {
    return 60.f + levelOf(UpgradeID::AUTO_COLLECT_RADIUS) * 30.f;
}

float GameState::oreLuckBonus() const {
    return levelOf(UpgradeID::ORE_LUCK) * 0.05f;
}

int GameState::plinkoRows() const {
    return std::min(PLINKO_MIN_ROWS + levelOf(UpgradeID::PLINKO_ROWS),
                    PLINKO_MAX_ROWS);
}

float GameState::plinkoMultBonus() const {
    return (1.f + levelOf(UpgradeID::PLINKO_MULT) * 0.10f)
           * _crystalPlinkoBonus();
}

int GameState::maxPlinkoBalls() const {
    return std::min(1 + levelOf(UpgradeID::PLINKO_BALLS),
                    MAX_PLINKO_BALLS);
}

float GameState::plinkoLuck() const {
    return levelOf(UpgradeID::PLINKO_LUCK) * 0.05f;
}

float GameState::creditMult() const {
    return (1.f + levelOf(UpgradeID::CREDIT_MULT) * 0.25f)
           * _crystalEconomyBonus();
}

int GameState::bulkProcess() const {
    return 1 + levelOf(UpgradeID::BULK_PROCESS);
}

bool GameState::autoPlinkoEnabled() const {
    return levelOf(UpgradeID::AUTO_PLINKO) > 0;
}

// ═════════════════════════════════════════════════════════════
//  Upgrade helpers
// ═════════════════════════════════════════════════════════════
int GameState::levelOf(UpgradeID id) const {
    return upgradeLevels[static_cast<int>(id)];
}
int GameState::levelOf(PrestigeUpgradeID id) const {
    return prestigeLevels[static_cast<int>(id)];
}

double GameState::costOf(UpgradeID id) const {
    const auto& def = upgradeCatalog[static_cast<int>(id)];
    return upgradeCost(def.baseCost, def.costMult, levelOf(id));
}
double GameState::costOf(PrestigeUpgradeID id) const {
    const auto& def = prestigeCatalog[static_cast<int>(id)];
    return upgradeCost(def.baseCost, def.costMult, levelOf(id));
}

bool GameState::canBuy(UpgradeID id) const {
    const auto& def = upgradeCatalog[static_cast<int>(id)];
    if (def.maxLevel > 0 && levelOf(id) >= def.maxLevel) return false;
    return credits >= costOf(id);
}
bool GameState::canBuy(PrestigeUpgradeID id) const {
    const auto& def = prestigeCatalog[static_cast<int>(id)];
    if (def.maxLevel > 0 && levelOf(id) >= def.maxLevel) return false;
    return crystals >= costOf(id);
}

void GameState::buy(UpgradeID id) {
    if (!canBuy(id)) return;
    credits -= costOf(id);
    upgradeLevels[static_cast<int>(id)]++;
}
void GameState::buy(PrestigeUpgradeID id) {
    if (!canBuy(id)) return;
    crystals -= costOf(id);
    prestigeLevels[static_cast<int>(id)]++;
}

// ═════════════════════════════════════════════════════════════
//  Prestige
// ═════════════════════════════════════════════════════════════
double GameState::crystalsOnPrestige() const {
    double gain = std::floor(std::sqrt(totalCredits / 1000.0));
    return std::max(gain, 1.0);
}

void GameState::doPrestige() {
    int keep = prestigeLevels[static_cast<int>(
        PrestigeUpgradeID::CRYSTAL_RETENTION)] * 2;

    crystals += crystalsOnPrestige();
    prestigeCount++;

    auto savedPrestige = prestigeLevels;

    std::array<std::pair<int,int>,
        static_cast<int>(UpgradeID::UPGRADE_COUNT)> ranked{};
    for (int i = 0; i < static_cast<int>(UpgradeID::UPGRADE_COUNT); i++)
        ranked[i] = { upgradeLevels[i], i };
    std::sort(ranked.begin(), ranked.end(), std::greater<>());

    reset();
    prestigeLevels = savedPrestige;

    for (int i = 0; i < keep &&
         i < static_cast<int>(UpgradeID::UPGRADE_COUNT); i++) {
        int idx = ranked[i].second;
        upgradeLevels[idx] = ranked[i].first / 2;
    }
}

// ═════════════════════════════════════════════════════════════
//  Reset
// ═════════════════════════════════════════════════════════════
void GameState::reset() {
    credits      = 0.0;
    ore          = 0.0;
    totalCredits = 0.0;
    totalOre     = 0.0;
    upgradeLevels.fill(0);
}

// ═════════════════════════════════════════════════════════════
//  Save / Load
// ═════════════════════════════════════════════════════════════
bool GameState::save(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    int ver = SAVE_VERSION;
    f.write(reinterpret_cast<const char*>(&ver),           sizeof(ver));
    f.write(reinterpret_cast<const char*>(&credits),       sizeof(credits));
    f.write(reinterpret_cast<const char*>(&ore),           sizeof(ore));
    f.write(reinterpret_cast<const char*>(&crystals),      sizeof(crystals));
    f.write(reinterpret_cast<const char*>(&totalCredits),  sizeof(totalCredits));
    f.write(reinterpret_cast<const char*>(&totalOre),      sizeof(totalOre));
    f.write(reinterpret_cast<const char*>(&prestigeCount), sizeof(prestigeCount));
    f.write(reinterpret_cast<const char*>(upgradeLevels.data()),
            upgradeLevels.size() * sizeof(int));
    f.write(reinterpret_cast<const char*>(prestigeLevels.data()),
            prestigeLevels.size() * sizeof(int));
    return f.good();
}

bool GameState::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    int ver = 0;
    f.read(reinterpret_cast<char*>(&ver), sizeof(ver));
    if (ver != SAVE_VERSION) return false;

    f.read(reinterpret_cast<char*>(&credits),       sizeof(credits));
    f.read(reinterpret_cast<char*>(&ore),           sizeof(ore));
    f.read(reinterpret_cast<char*>(&crystals),      sizeof(crystals));
    f.read(reinterpret_cast<char*>(&totalCredits),  sizeof(totalCredits));
    f.read(reinterpret_cast<char*>(&totalOre),      sizeof(totalOre));
    f.read(reinterpret_cast<char*>(&prestigeCount), sizeof(prestigeCount));
    f.read(reinterpret_cast<char*>(upgradeLevels.data()),
           upgradeLevels.size() * sizeof(int));
    f.read(reinterpret_cast<char*>(prestigeLevels.data()),
           prestigeLevels.size() * sizeof(int));
    return f.good();
}
