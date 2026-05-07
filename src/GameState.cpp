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
    // Weapons  (hogere basis, iets mildere mult t.o.v. economy — minder “gratis”)
    { "Gun Damage",        "+8 base damage per shot",         95.0,  1.52, 0 },
    { "Fire Rate",         "+0.4 shots/sec",                 140.0,  1.56, 0 },
    { "Turret Count",      "Add 1 turret",                   340.0,  2.08, 0 },
    { "Crit Chance",       "+5% crit chance",               190.0,  1.64, 0 },
    { "Crit Multiplier",   "+0.5x crit damage",             230.0,  1.68, 0 },
    { "Split Shot",        "Bullets split +1",               920.0,  2.28, 0 },
    // Mining
    { "Ore Value",         "+20% ore value",                 118.0,  1.57, 0 },
    { "Collect Radius",    "+30px collect radius",           102.0,  1.48, 0 },
    { "Ore Luck",          "+5% bonus ore drop",             130.0,  1.62, 0 },
    { "Asteroid HP",       "-10% asteroid HP",               260.0,  1.75, 0 },
    // Plinko
    { "Plinko Rows",       "Add 1 row to Plinko",            275.0,  1.92, 8  },
    { "Plinko Multiplier", "+10% slot multipliers",          215.0,  1.74, 0  },
    { "Plinko Balls",      "+1 max ball at once",            118.0,  1.42, 0  },
    { "Plinko Luck",       "+5% high-slot luck",             178.0,  1.64, 0  },
    // Economy  (iets zachter mult zodat mid-game niet vastloopt)
    { "Credit Multiplier", "+25% all credits",               198.0,  1.74, 0 },
    { "Bulk Processor",    "Convert more ore per drop",      315.0,  1.96, 0 },
    { "Auto-Plinko",       "Auto-drop balls",                540.0,  2.12, 0 },
    // Ore tier unlocks  (maxLevel = 1; vloeiendere curve, minder “muur” eindgame)
    { "Unlock Bronze",     "Spawn Bronze asteroids (3x ore)",     1400.0,  1.0, 1 },
    { "Unlock Silver",     "Spawn Silver asteroids (8x ore)",     6200.0,  1.0, 1 },
    { "Unlock Gold",       "Spawn Gold asteroids (20x ore)",     26000.0,  1.0, 1 },
    { "Unlock Diamond",    "Spawn Diamond asteroids (55x ore)", 105000.0, 1.0, 1 },
    { "Unlock Platinum",   "Spawn Platinum asteroids (140x)",   395000.0, 1.0, 1 },
    { "Unlock Titanium",   "Spawn Titanium asteroids (380x)",  1350000.0, 1.0, 1 },
    { "Unlock Iridium",    "Spawn Iridium asteroids (1000x)",  4500000.0, 1.0, 1 },
    // Travel
    { "Warp Drive",
      "Hold Space to warp to next zone (sneller per level; prijs schaalt per "
      "level)",
      35.0,
      1.42,
      5 },
    { "Bullet Range", "+8% bullet travel time / range", 88.0, 1.50, 0 },
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
      "Random pegs krijgen rarity bonus (+0.5x tot +8x); elk niveau +3 pegs",
      0 },
    { "Trough Boost",
      "Elk niveau: alle valbak-multipliers x1.32 extra (stapelt)",
      0 },
    { "Duplicator pegs",
      "Pegs die bij een bal-raak een extra bal spawnen (zelfde ore); "
      "meer niveaus = meer duplicator-pegs (rolls)",
      0 },
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

int GameState::chestDuplicatorRollCount() const {
    return levelOfChest(ChestUpgradeID::PLINKO_DUPLICATOR_PEG) * 3;
}

float GameState::chestPlinkoSlotMult() const {
    const int lv = levelOfChest(ChestUpgradeID::PLINKO_SLOT_MULT);
    if (lv <= 0)
        return 1.f;
    constexpr float kTroughPerLevel = 1.32f;
    return std::pow(kTroughPerLevel, static_cast<float>(lv));
}

int GameState::levelOfChest(ChestUpgradeID id) const {
    return chestLevels[static_cast<int>(id)];
}

int GameState::maxLives() const {
    switch (difficulty) {
        case Difficulty::Easy: return 4;
        case Difficulty::Medium: return 3;
        case Difficulty::Hard: return 2;
    }
    return 3;
}

float GameState::difficultyAsteroidHpMult() const {
    switch (difficulty) {
        case Difficulty::Easy: return 0.88f;
        case Difficulty::Medium: return 1.f;
        case Difficulty::Hard: return 1.14f;
    }
    return 1.f;
}

float GameState::hitInvulnerabilitySec() const {
    switch (difficulty) {
        case Difficulty::Easy: return 2.6f;
        case Difficulty::Medium: return 2.f;
        case Difficulty::Hard: return 1.45f;
    }
    return 2.f;
}

bool GameState::miningPausesWhenOffMiningTab() const {
    return difficulty == Difficulty::Easy;
}

bool GameState::showsRetreatToBaseOnOtherTabs() const {
    return difficulty == Difficulty::Medium;
}

float GameState::meteorShowerIntervalSec() const {
    switch (difficulty) {
        case Difficulty::Easy: return 42.f;
        case Difficulty::Medium: return 32.f;
        case Difficulty::Hard: return 24.f;
    }
    return 32.f;
}

int GameState::meteorShowerMeteorCount() const {
    switch (difficulty) {
        case Difficulty::Easy: return 6;
        case Difficulty::Medium: return 10;
        case Difficulty::Hard: return 14;
    }
    return 10;
}

// Elke chest kost precies 1 key (geen tier-prijs).
bool GameState::openOneChest(ChestUpgradeID* outChosen) {
    if (keys < 1)
        return false;
    ChestUpgradeID opts[static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT)];
    int            n = 0;
    for (int i = 0; i < static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT);
         ++i) {
        auto          id = static_cast<ChestUpgradeID>(i);
        const auto&   d  = chestCatalog[static_cast<int>(id)];
        const int     lv = levelOfChest(id);
        if (d.maxLevel > 0 && lv >= d.maxLevel)
            continue;
        opts[n++] = id;
    }
    if (n <= 0)
        return false;
    ChestUpgradeID pick = opts[randInt(0, n - 1)];
    --keys;
    chestLevels[static_cast<int>(pick)]++;
    if (outChosen)
        *outChosen = pick;
    return true;
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
           * _crystalDamageBonus();
}

float GameState::fireRatePerSec() const {
    return 1.5f + levelOf(UpgradeID::FIRE_RATE) * 0.4f;
}

int GameState::turretCount() const {
    return levelOf(UpgradeID::TURRET_COUNT);
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

float GameState::bulletLifetimeSec() const {
    constexpr float base = 2.8f;
    return base * (1.f + 0.08f * levelOf(UpgradeID::BULLET_RANGE));
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

double GameState::plinkoBallOreCost() const {
    const double o = std::max(0.0, ore);
    const double extra = std::max(0.0, o - 5000.0);
    return 1.0 + std::min(3.0, extra / 12000.0);
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
    const int   lv  = levelOf(id);
    if (id == UpgradeID::PLINKO_BALLS) {
        if (lv < 20)
            return 4.0 * std::pow(1.24, static_cast<double>(lv));
        return upgradeCost(def.baseCost, def.costMult, std::max(0, lv - 14));
    }
    return upgradeCost(def.baseCost, def.costMult, lv);
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

float GameState::warpDurationSec() const {
    const int lv = levelOf(UpgradeID::WARP_DRIVE);
    constexpr float durations[] = { 16.f, 13.f, 10.f, 8.f, 6.f, 3.f };
    const int       idx         = std::clamp(lv, 0, 5);
    return durations[idx];
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

    const Difficulty   savedDiff = difficulty;
    const int          savedKeys = keys;
    const auto         savedChest = chestLevels;
    const auto         savedUnlock = unlockPhaseDone;
    const bool         savedKeyAst = keyAsteroidsEnabled;

    crystals += crystalsOnPrestige();
    prestigeCount++;

    auto savedPrestige = prestigeLevels;

    std::array<std::pair<int,int>,
        static_cast<int>(UpgradeID::UPGRADE_COUNT)> ranked{};
    for (int i = 0; i < static_cast<int>(UpgradeID::UPGRADE_COUNT); i++)
        ranked[i] = { upgradeLevels[i], i };
    std::sort(ranked.begin(), ranked.end(), std::greater<>());

    reset();
    difficulty          = savedDiff;
    keys                = savedKeys;
    chestLevels         = savedChest;
    prestigeLevels      = savedPrestige;
    unlockPhaseDone     = savedUnlock;
    keyAsteroidsEnabled = savedKeyAst;
    lives               = maxLives();

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
    difficulty   = Difficulty::Medium;
    lives        = maxLives();
    keys         = 0;
    chestLevels.fill(0);
    nextBossMilestone = 3;
    bossCrystalPopup  = 0.0;
    unlockPhaseDone.fill(false);
    keyAsteroidsEnabled = false;
}

void GameState::migrateUnlockProgressFromLegacyState() {
    unlockPhaseDone.fill(false);
    if (totalOre >= 5.0)
        unlockPhaseDone[1] = true;
    if (totalOre >= 25.0)
        unlockPhaseDone[2] = true;
    if (currentLevel >= 2)
        unlockPhaseDone[3] = true;
    if (credits >= 50.0)
        unlockPhaseDone[4] = true;
    if (credits >= 100.0)
        unlockPhaseDone[5] = true;
    if (currentLevel >= 3) {
        unlockPhaseDone[6] = true;
        unlockPhaseDone[7] = true;
    }
    if (currentLevel >= 5)
        unlockPhaseDone[8] = true;
    keyAsteroidsEnabled = (currentLevel >= 3);
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
    const uint8_t diffByte = static_cast<uint8_t>(difficulty);
    f.write(reinterpret_cast<const char*>(&diffByte), sizeof(diffByte));
    for (bool b : unlockPhaseDone) {
        const uint8_t byte = b ? 1u : 0u;
        f.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
    }
    const uint8_t keyAst = keyAsteroidsEnabled ? 1u : 0u;
    f.write(reinterpret_cast<const char*>(&keyAst), sizeof(keyAst));
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
    if (ver < 13) {
        constexpr int legacySlots =
            static_cast<int>(UpgradeID::UPGRADE_COUNT) - 1;
        f.read(reinterpret_cast<char*>(upgradeLevels.data()),
               legacySlots * sizeof(int));
        upgradeLevels[legacySlots] = 0;
    } else {
        f.read(reinterpret_cast<char*>(upgradeLevels.data()),
               upgradeLevels.size() * sizeof(int));
    }
    f.read(reinterpret_cast<char*>(prestigeLevels.data()),
           prestigeLevels.size() * sizeof(int));
    chestLevels.fill(0);
    if (ver >= 7) {
        if (ver >= 12) {
            f.read(reinterpret_cast<char*>(chestLevels.data()),
                   chestLevels.size() * sizeof(int));
        } else if (ver >= 10) {
            int c0 = 0, c1 = 0;
            f.read(reinterpret_cast<char*>(&c0), sizeof(c0));
            f.read(reinterpret_cast<char*>(&c1), sizeof(c1));
            chestLevels[0] = c0;
            chestLevels[1] = c1;
            chestLevels[static_cast<int>(ChestUpgradeID::PLINKO_DUPLICATOR_PEG)] =
                0;
        } else {
            std::array<int, 4> oldChest{};
            f.read(reinterpret_cast<char*>(oldChest.data()),
                   oldChest.size() * sizeof(int));
            chestLevels[0] = oldChest[0];
            chestLevels[1] = oldChest[1];
            chestLevels[static_cast<int>(ChestUpgradeID::PLINKO_DUPLICATOR_PEG)] =
                0;
        }
    }
    nextBossMilestone = 3;
    if (ver >= 8)
        f.read(reinterpret_cast<char*>(&nextBossMilestone),
               sizeof(nextBossMilestone));
    difficulty = Difficulty::Medium;
    if (ver >= 9) {
        uint8_t db = 0;
        f.read(reinterpret_cast<char*>(&db), sizeof(db));
        if (db <= static_cast<uint8_t>(Difficulty::Hard))
            difficulty = static_cast<Difficulty>(db);
    }
    unlockPhaseDone.fill(false);
    keyAsteroidsEnabled = false;
    if (ver >= 14) {
        for (std::size_t i = 0; i < unlockPhaseDone.size(); ++i) {
            uint8_t byte = 0;
            f.read(reinterpret_cast<char*>(&byte), sizeof(byte));
            unlockPhaseDone[i] = (byte != 0);
        }
        uint8_t keyAst = 0;
        f.read(reinterpret_cast<char*>(&keyAst), sizeof(keyAst));
        keyAsteroidsEnabled = (keyAst != 0);
    } else
        migrateUnlockProgressFromLegacyState();
    if (lives > maxLives())
        lives = maxLives();
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
    lives        = maxLives();
    // upgrades blijven staan
}
