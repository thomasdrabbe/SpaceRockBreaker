#include "GameState.h"
#include "Utils.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <climits>

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
    { "Plinko Balls",      "+1 max ball at once",            1.0,  1, 0  },
    { "Plinko Luck",       "+5% high-slot luck",             160.0,  1.70, 0  },
    // Economy
    { "Credit Multiplier", "+25% all credits",               250.0,  1.90, 0 },
    { "Bulk Processor",    "Convert more ore per drop",      400.0,  2.10, 0 },
    { "Auto-Plinko",       "Auto-drop balls",                750.0,  2.30, 0 },
    // Ore tier unlocks  (maxLevel = 1 = one-time purchase)
    { "Unlock Bronze",     "Spawn Bronze asteroids (3x ore)",    1500.0,  1.0, 1 },
    { "Unlock Silver",     "Spawn Silver asteroids (8x ore)",    8000.0,  1.0, 1 },
    { "Unlock Gold",       "Spawn Gold asteroids (20x ore)",    40000.0,  1.0, 1 },
    { "Unlock Diamond",    "Spawn Diamond asteroids (55x ore)", 200000.0, 1.0, 1 },
    { "Unlock Platinum",   "Spawn Platinum asteroids (140x)",   800000.0, 1.0, 1 },
    { "Unlock Titanium",   "Spawn Titanium asteroids (380x)",  3000000.0, 1.0, 1 },
    { "Unlock Iridium",    "Spawn Iridium asteroids (1000x)", 12000000.0, 1.0, 1 },
    // Travel
    { "Warp Drive", "Hold Space to warp to next zone", 500.0, 1.0, 1 },
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

const std::array<ChestDef, static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT)>
GameState::chestCatalog = {{
    { "Golden Pegs",
      "Random pegs krijgen rarity bonus (+0.5x tot +8x)",
      2, 1.22, 20 },
    { "Reactive Pegs",   "Peg bounce strength +3% / level",    2, 1.22, 12 },
    { "Ammo Cache",      "Gun damage +1.5 / level",             3, 1.28, 10 },
    { "Refinery Pass",   "Ore value +2% / level",               3, 1.28, 12 },
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
//  Chest upgrades
// ═════════════════════════════════════════════════════════════
int GameState::chestPegUpgradeCount() const {
    return levelOfChest(ChestUpgradeID::PLINKO_PEG_SIZE) * 3;
}

float GameState::chestPlinkoBounceMult() const {
    int lv = levelOfChest(ChestUpgradeID::PLINKO_PEG_BOUNCE);
    return 1.f + 0.03f * static_cast<float>(lv);
}

float GameState::chestGunFlatBonus() const {
    return 1.5f * static_cast<float>(
        levelOfChest(ChestUpgradeID::GUN_FLAT_DAMAGE));
}

float GameState::chestOreValueMult() const {
    return 1.f + 0.02f * static_cast<float>(
        levelOfChest(ChestUpgradeID::MINING_ORE_VALUE));
}

int GameState::levelOfChest(ChestUpgradeID id) const {
    return chestLevels[static_cast<int>(id)];
}

int GameState::keyCostOf(ChestUpgradeID id) const {
    const auto& d = chestCatalog[static_cast<int>(id)];
    int         lv  = levelOfChest(id);
    if (d.maxLevel > 0 && lv >= d.maxLevel)
        return INT_MAX;
    return static_cast<int>(std::ceil(
        static_cast<double>(d.baseKeyCost)
        * std::pow(d.keyCostMult, static_cast<double>(lv))));
}

bool GameState::canBuyChest(ChestUpgradeID id) const {
    int c = keyCostOf(id);
    if (c == INT_MAX) return false;
    return keys >= c;
}

void GameState::buyChest(ChestUpgradeID id) {
    if (!canBuyChest(id)) return;
    keys -= keyCostOf(id);
    chestLevels[static_cast<int>(id)]++;
}

// ═════════════════════════════════════════════════════════════
//  warp drive requirments
// ═════════════════════════════════════════════════════════════
int GameState::oreWarpRequirement() const {
    return 5 * currentLevel * (currentLevel + 1);
}

// ═════════════════════════════════════════════════════════════
//  Computed stats
// ═════════════════════════════════════════════════════════════
float GameState::gunDamage() const {
    return (10.f + levelOf(UpgradeID::GUN_DAMAGE) * 8.f)
           * _crystalDamageBonus()
           + chestGunFlatBonus();
}

float GameState::fireRatePerSec() const {
    return 1.5f + levelOf(UpgradeID::FIRE_RATE) * 0.4f;
}

int GameState::turretCount() const {
    return + levelOf(UpgradeID::TURRET_COUNT);
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
           * _crystalMiningBonus()
           * chestOreValueMult();
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
//  Ore tier / level helpers
// ═════════════════════════════════════════════════════════════
OreTier GameState::maxOreTier() const {
    // Each unlock upgrade adds one tier step
    // IRON is always available; each unlock shifts the cap up
    const UpgradeID unlocks[] = {
        UpgradeID::UNLOCK_BRONZE,
        UpgradeID::UNLOCK_SILVER,
        UpgradeID::UNLOCK_GOLD,
        UpgradeID::UNLOCK_DIAMOND,
        UpgradeID::UNLOCK_PLATINUM,
        UpgradeID::UNLOCK_TITANIUM,
        UpgradeID::UNLOCK_IRIDIUM,
    };
    int tier = 0;
    for (auto id : unlocks) {
        if (levelOf(id) > 0) tier++;
        else break;  // tiers must be bought in order
    }
    return static_cast<OreTier>(tier);
}

float GameState::levelHpMult() const {
    // +15% asteroid HP per zone level
    return 1.f + (currentLevel - 1) * 0.15f;
}

int GameState::levelSpawnBonus() const {
    // +2 extra target asteroids per zone level
    return (currentLevel - 1) * 2;
}

std::string GameState::levelLabel() const {
    return "Zone " + std::to_string(currentLevel);
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

bool GameState::warpDriveUnlocked() const {
    return levelOf(UpgradeID::WARP_DRIVE) > 0;
}

bool GameState::canWarp() const {
    return warpDriveUnlocked() && oreThisLevel >= oreWarpRequirement();
}

void GameState::doWarp() {
    currentLevel++;
    oreThisLevel = 0.0;
}

namespace {

int nextBossZoneAfter(int beatenZone) {
    static const int seq[] = {
        3,  5,  10, 15, 20, 25, 30, 35, 40, 45, 50,
        60, 70, 80, 90, 100, 115, 130, 150, 170, 200
    };
    for (int s : seq) {
        if (s > beatenZone)
            return s;
    }
    return beatenZone + 25;
}

} // namespace

void GameState::registerBossDefeated() {
    const int z = nextBossMilestone;
    double bonus = 6.0 + static_cast<double>(z) * 2.0
                 + std::floor(std::sqrt(static_cast<double>(z * z)));
    const double gain = std::max(8.0, bonus);
    crystals         += gain;
    bossCrystalPopup  = gain;
    nextBossMilestone = nextBossZoneAfter(z);
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

    const int savedKeys = keys;
    const auto savedChest = chestLevels;

    crystals += crystalsOnPrestige();
    prestigeCount++;

    auto savedPrestige = prestigeLevels;

    std::array<std::pair<int,int>,
        static_cast<int>(UpgradeID::UPGRADE_COUNT)> ranked{};
    for (int i = 0; i < static_cast<int>(UpgradeID::UPGRADE_COUNT); i++)
        ranked[i] = { upgradeLevels[i], i };
    std::sort(ranked.begin(), ranked.end(), std::greater<>());

    reset();
    keys           = savedKeys;
    chestLevels    = savedChest;
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
    currentLevel = 1;          // ← nieuw
    upgradeLevels.fill(0);
    oreThisLevel = 0.0;
    lives        = MAX_LIVES;
    keys         = 0;
    chestLevels.fill(0);
    nextBossMilestone = 3;
    bossCrystalPopup  = 0.0;
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
    f.write(reinterpret_cast<const char*>(&currentLevel), sizeof(currentLevel));
    f.write(reinterpret_cast<const char*>(&oreThisLevel), sizeof(oreThisLevel));
    f.write(reinterpret_cast<const char*>(&lives), sizeof(lives));
    f.write(reinterpret_cast<const char*>(&keys), sizeof(keys));
    f.write(reinterpret_cast<const char*>(upgradeLevels.data()),
            upgradeLevels.size() * sizeof(int));
    f.write(reinterpret_cast<const char*>(prestigeLevels.data()),
            prestigeLevels.size() * sizeof(int));
    f.write(reinterpret_cast<const char*>(chestLevels.data()),
            chestLevels.size() * sizeof(int));
    f.write(reinterpret_cast<const char*>(&nextBossMilestone),
            sizeof(nextBossMilestone));
    return f.good();
}

bool GameState::peekSaveSlot(const std::string& path,
                             int&         outZone,
                             double&      outCredits) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;

    int ver = 0;
    f.read(reinterpret_cast<char*>(&ver), sizeof(ver));
    if (ver < 5 || ver > SAVE_VERSION)
        return false;

    double credits = 0.0, ore = 0.0, cry = 0.0, tc = 0.0, to = 0.0;
    int    pres = 0, zone = 0, lives = 0;
    double orel = 0.0;
    f.read(reinterpret_cast<char*>(&credits), sizeof(credits));
    f.read(reinterpret_cast<char*>(&ore), sizeof(ore));
    f.read(reinterpret_cast<char*>(&cry), sizeof(cry));
    f.read(reinterpret_cast<char*>(&tc), sizeof(tc));
    f.read(reinterpret_cast<char*>(&to), sizeof(to));
    f.read(reinterpret_cast<char*>(&pres), sizeof(pres));
    f.read(reinterpret_cast<char*>(&zone), sizeof(zone));
    f.read(reinterpret_cast<char*>(&orel), sizeof(orel));
    f.read(reinterpret_cast<char*>(&lives), sizeof(lives));
    if (ver >= 6) {
        int keysDummy = 0;
        f.read(reinterpret_cast<char*>(&keysDummy), sizeof(keysDummy));
    }
    if (!f.good())
        return false;
    outZone     = zone;
    outCredits  = credits;
    return true;
}

bool GameState::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    int ver = 0;
    f.read(reinterpret_cast<char*>(&ver), sizeof(ver));
    if (ver < 5 || ver > SAVE_VERSION) return false;

    f.read(reinterpret_cast<char*>(&credits),       sizeof(credits));
    f.read(reinterpret_cast<char*>(&ore),           sizeof(ore));
    f.read(reinterpret_cast<char*>(&crystals),      sizeof(crystals));
    f.read(reinterpret_cast<char*>(&totalCredits),  sizeof(totalCredits));
    f.read(reinterpret_cast<char*>(&totalOre),      sizeof(totalOre));
    f.read(reinterpret_cast<char*>(&prestigeCount), sizeof(prestigeCount));
    f.read(reinterpret_cast<char*>(&currentLevel), sizeof(currentLevel));
    f.read(reinterpret_cast<char*>(&oreThisLevel), sizeof(oreThisLevel));
    f.read(reinterpret_cast<char*>(&lives), sizeof(lives));
    keys = 0;
    if (ver >= 6)
        f.read(reinterpret_cast<char*>(&keys), sizeof(keys));
    f.read(reinterpret_cast<char*>(upgradeLevels.data()),
           upgradeLevels.size() * sizeof(int));
    f.read(reinterpret_cast<char*>(prestigeLevels.data()),
           prestigeLevels.size() * sizeof(int));
    chestLevels.fill(0);
    if (ver >= 7) {
        f.read(reinterpret_cast<char*>(chestLevels.data()),
               chestLevels.size() * sizeof(int));
    }
    nextBossMilestone = 3;
    if (ver >= 8)
        f.read(reinterpret_cast<char*>(&nextBossMilestone),
               sizeof(nextBossMilestone));
    return f.good();
}
// ── game over ─────────────────────────────────────────────

void GameState::loseLife() {
    lives--;
    if (lives < 0) lives = 0;
}

void GameState::gameOver() {
    credits      = 0.0;
    ore          = 0.0;
    oreThisLevel = 0.0;
    currentLevel = 1;
    lives        = MAX_LIVES;
    // upgrades blijven staan
}
