#include "GameState.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <fstream>
#include <limits>

namespace {

[[nodiscard]] bool upgradeIdInRange(UpgradeID id) noexcept {
    const int i = static_cast<int>(id);
    return i >= 0 && i < static_cast<int>(UpgradeID::UPGRADE_COUNT);
}

[[nodiscard]] bool prestigeIdInRange(PrestigeUpgradeID id) noexcept {
    const int i = static_cast<int>(id);
    return i >= 0
           && i < static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT);
}

[[nodiscard]] bool chestIdInRange(ChestUpgradeID id) noexcept {
    const int i = static_cast<int>(id);
    return i >= 0 && i < static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT);
}

} // namespace

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
    { "Auto-Plinko",
      "Lv1: unlock auto-drop; elk extra level +1 bal per tick (zelfde interval)",
      540.0,
      2.12,
      0 },
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
static_assert(
    GameState::upgradeCatalog.size()
        == static_cast<std::size_t>(UpgradeID::UPGRADE_COUNT),
    "upgradeCatalog must have exactly one entry per UpgradeID value");

const std::array<PrestigeUpgradeDef,
    static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT)>
GameState::prestigeCatalog = {{
    { "Crystal Damage",  "+15% gun damage permanently",  1.0, 1.80, 0 },
    { "Crystal Mining",  "+15% ore value permanently",   1.0, 1.80, 0 },
    { "Crystal Economy", "+15% all credits permanently", 1.0, 1.80, 0 },
    { "Crystal Plinko",  "+15% plinko multipliers",      1.0, 1.80, 0 },
    { "Deep Retention",  "Keep 2 extra upgrades/level",  3.0, 2.50, 0 },
}};
static_assert(GameState::prestigeCatalog.size()
                  == static_cast<std::size_t>(
                      PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT),
              "prestigeCatalog must match PrestigeUpgradeID count");

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
static_assert(
    GameState::chestCatalog.size()
        == static_cast<std::size_t>(ChestUpgradeID::CHEST_UPGRADE_COUNT),
    "chestCatalog must match ChestUpgradeID count");

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
    if (!chestIdInRange(id))
        return 0;
    return chestLevels[static_cast<std::size_t>(static_cast<int>(id))];
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

void GameState::addCredits(double amount) {
    if (amount <= 0.0)
        return;
    credits += amount;
    totalCredits += amount;
}

bool GameState::spendCredits(double amount) {
    if (amount <= 0.0)
        return true;
    if (credits + 1e-9 < amount)
        return false;
    credits -= amount;
    if (credits < 0.0)
        credits = 0.0;
    return true;
}

void GameState::addOre(double amount, bool countForWarp) {
    if (amount <= 0.0)
        return;
    ore += amount;
    totalOre += amount;
    orePerTier[static_cast<int>(OreTier::IRON)] += amount;
    if (countForWarp)
        oreThisLevel += amount;
}

void GameState::addOreTiered(
    const std::array<double, ORE_TIER_COUNT>& oreByTier,
    bool countForWarp) {
    double sum = 0.0;
    for (int i = 0; i < ORE_TIER_COUNT; ++i) {
        const double add = std::max(0.0, oreByTier[static_cast<std::size_t>(i)]);
        orePerTier[static_cast<std::size_t>(i)] += add;
        sum += add;
    }
    if (sum <= 0.0)
        return;
    ore += sum;
    totalOre += sum;
    if (countForWarp)
        oreThisLevel += sum;
}

bool GameState::spendOre(double amount) {
    if (amount <= 0.0)
        return true;
    if (ore + 1e-9 < amount)
        return false;
    double remain = amount;
    for (int i = ORE_TIER_COUNT - 1; i >= 0 && remain > 1e-9; --i) {
        auto& bucket = orePerTier[static_cast<std::size_t>(i)];
        if (bucket <= 0.0)
            continue;
        const double take = std::min(bucket, remain);
        bucket -= take;
        remain -= take;
    }
    if (remain > 1e-9) {
        auto& iron = orePerTier[static_cast<int>(OreTier::IRON)];
        iron = std::max(0.0, iron - remain);
    }
    ore -= amount;
    if (ore < 0.0)
        ore = 0.0;
    return true;
}

void GameState::addCrystals(double amount) {
    if (amount <= 0.0)
        return;
    crystals += amount;
}

void GameState::addKeys(int amount) {
    if (amount <= 0)
        return;
    keys += amount;
}

bool GameState::consumeKeys(int amount) {
    if (amount <= 0)
        return true;
    if (keys < amount)
        return false;
    keys -= amount;
    return true;
}

OreTier GameState::dominantOreTier() const {
    int bestIdx = 0;
    double best = -1.0;
    for (int i = 0; i < ORE_TIER_COUNT; ++i) {
        const double amt = orePerTier[static_cast<std::size_t>(i)];
        if (amt > best + 1e-9) {
            best = amt;
            bestIdx = i;
        }
    }
    return static_cast<OreTier>(bestIdx);
}

sf::Color GameState::dominantOreColor() const {
    return oreTierColor(dominantOreTier());
}

bool GameState::deductOneOre(OreTier& outTier) {
    return spendOreForPlinko(1.0, outTier);
}

bool GameState::spendOreForPlinko(double amount, OreTier& outTier) {
    if (amount <= 0.0)
        return true;
    if (ore + 1e-9 < amount)
        return false;

    double remain = amount;
    bool firstTierAssigned = false;
    for (int i = ORE_TIER_COUNT - 1; i >= 0 && remain > 1e-9; --i) {
        auto& bucket = orePerTier[static_cast<std::size_t>(i)];
        if (bucket <= 1e-9)
            continue;
        if (!firstTierAssigned) {
            outTier = static_cast<OreTier>(i);
            firstTierAssigned = true;
        }
        const double take = std::min(bucket, remain);
        bucket -= take;
        remain -= take;
    }
    if (remain > 1e-9) {
        auto& iron = orePerTier[static_cast<int>(OreTier::IRON)];
        iron = std::max(0.0, iron - remain);
    }
    if (!firstTierAssigned)
        outTier = OreTier::IRON;
    ore -= amount;
    if (ore < 0.0)
        ore = 0.0;
    return true;
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
        if (!chestIdInRange(id))
            continue;
        const auto& d = chestCatalog[static_cast<std::size_t>(i)];
        const int     lv = levelOfChest(id);
        if (d.maxLevel > 0 && lv >= d.maxLevel)
            continue;
        opts[n++] = id;
    }
    if (n <= 0)
        return false;
    ChestUpgradeID pick = opts[randInt(0, n - 1)];
    consumeKeys(1);
    chestLevels[static_cast<int>(pick)]++;
    if (outChosen)
        *outChosen = pick;
    return true;
}

// ═════════════════════════════════════════════════════════════
//  warp drive requirments
// ═════════════════════════════════════════════════════════════
int GameState::oreWarpRequirement() const {
    const std::int64_t z = static_cast<std::int64_t>(currentLevel);
    const std::int64_t p = 5 * z * (z + 1);
    if (p > std::numeric_limits<int>::max())
        return std::numeric_limits<int>::max();
    if (p < 1)
        return 1;
    return static_cast<int>(p);
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
           * _crystalMiningBonus() * bonusZoneOreValueMult();
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

int GameState::autoPlinkoBallsPerTick() const {
    const int lv = levelOf(UpgradeID::AUTO_PLINKO);
    return lv > 0 ? lv : 0;
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
    const std::int64_t z = static_cast<std::int64_t>(currentLevel);
    const std::int64_t b = (z - 1) * 2;
    constexpr int        kCap = 500;
    if (b >= kCap)
        return kCap;
    if (b <= 0)
        return 0;
    return static_cast<int>(b);
}

std::string GameState::levelLabel() const {
    // ASCII separator: sommige systemen/fonts crashen of falen op U+2014 (em dash).
    return "Zone " + std::to_string(currentLevel) + " - " + currentZoneName();
}

OreTier GameState::bonusZoneMinOreTier() const {
    switch (bonusZoneRarity) {
        case OreRarity::EPIC:
            return OreTier::GOLD;
        case OreRarity::MYTHIC:
            return OreTier::DIAMOND;
        case OreRarity::LEGENDARY:
            return OreTier::IRIDIUM;
        default:
            return OreTier::IRON;
    }
}

float GameState::bonusZoneOreValueMult() const {
    if (!isBonusZone)
        return 1.0f;
    switch (bonusZoneRarity) {
        case OreRarity::UNCOMMON:
            return 1.5f;
        case OreRarity::RARE:
        case OreRarity::EPIC:
        case OreRarity::MYTHIC:
        case OreRarity::LEGENDARY:
            return 2.0f;
        default:
            return 1.0f;
    }
}

OreTier GameState::clampKeyOreTier(OreTier preferred, OreTier maxUnlocked) {
    return static_cast<OreTier>(
        std::clamp(static_cast<int>(preferred), static_cast<int>(OreTier::IRON),
                   static_cast<int>(maxUnlocked)));
}

namespace {

const char* const ZONE_PREFIXES[] = {
    "Nebula",        "Asteroid Belt", "Void",       "Sector",     "Expanse",
    "Cluster",       "Rift",          "Drift",      "Remnant",    "Frontier",
    "Abyss",         "Cradle",        "Forge",      "Storm",      "Silence",
    "Pulsar",        "Quasar",        "Corona",     "Veil",       "Shroud",
    "Reach",         "Basin",         "Hollow",     "Tempest",    "Cascade",
};
const char* const ZONE_PLANETS[] = {
    "Valdris",  "Korrath",  "Zephyra",  "Thunex",   "Calyx",
    "Myndor",   "Errath",   "Solven",   "Kaelthas", "Vorryn",
    "Duskara",  "Nhelox",   "Pyreth",   "Caelum",   "Zorvan",
    "Ulthar",   "Xyndra",   "Beldrox",  "Tyrannis", "Ossian",
    "Quelith",  "Ravorn",   "Solvex",   "Tharyn",   "Uxelos",
    "Vryndal",  "Wethyx",   "Xarkon",   "Ysolde",   "Zanthos",
};
const char* const ZONE_SUFFIXES[] = {
    "Prime",    "Minor",    "Major",    "IX",       "VII",
    "IV",       "II",       "Zero",     "Alpha",    "Omega",
    "Core",     "Rim",      "Deep",     "Reach",    "Far",
    "Lost",     "Ancient",  "Forgotten", "End",
};

constexpr int kZonePrefixCount = 25;
constexpr int kZonePlanetCount = 30;
constexpr int kZoneSuffixCount = 19;

} // namespace

std::string GameState::zoneNameFor(int zone) const {
    const unsigned uz =
        static_cast<unsigned>(std::max(1, zone));
    // Alleen unsigned modulo: cast naar `int` vóór `%` kan negatief worden
    // (unsigned → signed overflow), dan is `i % n` in C++ negatief en wordt
    // `(size_t)i` een enorme index → UB / crashes in string-ops — sinds bonus
    // zones + zoneNameFor zichtbaar op veel paden (HUD, warp, notificaties).
    auto pick = [&](int arraySize, unsigned salt) -> int {
        if (arraySize <= 0)
            return 0;
        const unsigned mix = (uz * 2654435761u) ^ (salt * 2246822519u);
        return static_cast<int>(mix % static_cast<unsigned>(arraySize));
    };

    std::string prefix;
    if (isBonusZone) {
        static const char* const rarityPrefixes[] = {
            "",         "Rich ",      "Fertile ",   "Bountiful ",
            "Sacred ",  "Legendary ",
        };
        static_assert(
            std::size(rarityPrefixes)
                == static_cast<std::size_t>(OreRarity::LEGENDARY) + 1u,
            "rarityPrefixes must align with OreRarity COMMON..LEGENDARY");
        const int ri =
            std::clamp(static_cast<int>(bonusZoneRarity), 0,
                       static_cast<int>(OreRarity::LEGENDARY));
        prefix = strFromNullableUtf8(
            rarityPrefixes[static_cast<std::size_t>(ri)]);
    }

    const std::string type = strFromNullableUtf8(
        ZONE_PREFIXES[static_cast<std::size_t>(pick(kZonePrefixCount, 1u))]);
    const std::string planet = strFromNullableUtf8(
        ZONE_PLANETS[static_cast<std::size_t>(pick(kZonePlanetCount, 2u))]);
    const std::string suffix = strFromNullableUtf8(
        ZONE_SUFFIXES[static_cast<std::size_t>(pick(kZoneSuffixCount, 3u))]);

    return prefix + type + " " + planet + "-" + suffix;
}

std::string GameState::currentZoneName() const {
    return zoneNameFor(currentLevel);
}

OreRarity GameState::rollBonusZoneRarity() {
    constexpr float weights[] = {
        50.0f, 25.0f, 14.0f, 7.5f, 2.8f, 0.7f,
    };
    float total = 0.f;
    for (float w : weights)
        total += w;
    float roll = randFloat(0.f, total);
    float cum  = 0.f;
    for (int i = 0; i < 6; ++i) {
        cum += weights[i];
        if (roll < cum)
            return static_cast<OreRarity>(i);
    }
    return OreRarity::COMMON;
}

// ═════════════════════════════════════════════════════════════
//  Upgrade helpers
// ═════════════════════════════════════════════════════════════
int GameState::levelOf(UpgradeID id) const {
    if (!upgradeIdInRange(id))
        return 0;
    return upgradeLevels[static_cast<std::size_t>(static_cast<int>(id))];
}
int GameState::levelOf(PrestigeUpgradeID id) const {
    if (!prestigeIdInRange(id))
        return 0;
    return prestigeLevels[static_cast<std::size_t>(static_cast<int>(id))];
}

double GameState::costOf(UpgradeID id) const {
    if (!upgradeIdInRange(id))
        return std::numeric_limits<double>::infinity();
    const auto& def = upgradeCatalog[static_cast<std::size_t>(static_cast<int>(id))];
    const int   lv  = levelOf(id);
    if (id == UpgradeID::PLINKO_BALLS) {
        if (lv < 20)
            return 4.0 * std::pow(1.24, static_cast<double>(lv));
        return upgradeCost(def.baseCost, def.costMult, std::max(0, lv - 14));
    }
    return upgradeCost(def.baseCost, def.costMult, lv);
}
double GameState::costOf(PrestigeUpgradeID id) const {
    if (!prestigeIdInRange(id))
        return std::numeric_limits<double>::infinity();
    const auto& def =
        prestigeCatalog[static_cast<std::size_t>(static_cast<int>(id))];
    return upgradeCost(def.baseCost, def.costMult, levelOf(id));
}

bool GameState::canBuy(UpgradeID id) const {
    if (!upgradeIdInRange(id))
        return false;
    const auto& def = upgradeCatalog[static_cast<std::size_t>(static_cast<int>(id))];
    if (def.maxLevel > 0 && levelOf(id) >= def.maxLevel) return false;
    return credits >= costOf(id);
}
bool GameState::canBuy(PrestigeUpgradeID id) const {
    if (!prestigeIdInRange(id))
        return false;
    const auto& def =
        prestigeCatalog[static_cast<std::size_t>(static_cast<int>(id))];
    if (def.maxLevel > 0 && levelOf(id) >= def.maxLevel) return false;
    return crystals >= costOf(id);
}

bool GameState::warpDriveUnlocked() const {
    return levelOf(UpgradeID::WARP_DRIVE) > 0;
}

float GameState::warpDurationSec() const {
    const int lv = levelOf(UpgradeID::WARP_DRIVE);
    // Warp Drive is pas bruikbaar vanaf lv1; lv1 moet 16s blijven voor juiste timing.
    constexpr float durations[] = { 16.f, 16.f, 13.f, 10.f, 8.f, 6.f };
    const int       idx         = std::clamp(lv, 0, 5);
    return durations[idx];
}

bool GameState::canWarp() const {
    return warpDriveUnlocked() && oreThisLevel >= oreWarpRequirement();
}

void GameState::doWarp() {
    if (isBonusZone) {
        currentLevel++;
        isBonusZone     = false;
        bonusZoneRarity = OreRarity::COMMON;
        oreThisLevel    = 0.0;
        return;
    }

    oreThisLevel = 0.0;

    const float bonusChance = 0.20f + oreLuckBonus() * 0.05f;
    if (randFloat(0.f, 1.f) < bonusChance) {
        isBonusZone     = true;
        bonusZoneRarity = rollBonusZoneRarity();
    } else {
        currentLevel++;
        isBonusZone     = false;
        bonusZoneRarity = OreRarity::COMMON;
    }
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
    addCrystals(gain);
    bossCrystalPopup  = gain;
    nextBossMilestone = nextBossZoneAfter(z);
}

void GameState::buy(UpgradeID id) {
    if (!upgradeIdInRange(id) || !canBuy(id)) return;
    spendCredits(costOf(id));
    upgradeLevels[static_cast<std::size_t>(static_cast<int>(id))]++;
}
void GameState::buy(PrestigeUpgradeID id) {
    if (!prestigeIdInRange(id) || !canBuy(id)) return;
    crystals = std::max(0.0, crystals - costOf(id));
    prestigeLevels[static_cast<std::size_t>(static_cast<int>(id))]++;
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

    addCrystals(crystalsOnPrestige());
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
    crystals     = 0.0;
    totalCredits = 0.0;
    totalOre     = 0.0;
    orePerTier.fill(0.0);
    currentLevel = 1;          // ← nieuw
    upgradeLevels.fill(0);
    prestigeLevels.fill(0);
    prestigeCount = 0;
    oreThisLevel = 0.0;
    difficulty   = Difficulty::Medium;
    lives        = maxLives();
    keys         = 0;
    chestLevels.fill(0);
    nextBossMilestone = 3;
    bossCrystalPopup  = 0.0;
    unlockPhaseDone.fill(false);
    keyAsteroidsEnabled = false;
    isBonusZone        = false;
    bonusZoneRarity     = OreRarity::COMMON;
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
    f.write(reinterpret_cast<const char*>(orePerTier.data()),
            orePerTier.size() * sizeof(double));
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
    const uint8_t bonusB = isBonusZone ? 1u : 0u;
    f.write(reinterpret_cast<const char*>(&bonusB), sizeof(bonusB));
    const uint8_t bonusR = static_cast<uint8_t>(bonusZoneRarity);
    f.write(reinterpret_cast<const char*>(&bonusR), sizeof(bonusR));
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

namespace {

void sanitizeLoadedState(GameState& s) {
    if (s.currentLevel < 1)
        s.currentLevel = 1;
    constexpr int kMaxZone = 500'000;
    if (s.currentLevel > kMaxZone)
        s.currentLevel = kMaxZone;

    if (s.keys < 0)
        s.keys = 0;

    if (s.nextBossMilestone < 3)
        s.nextBossMilestone = 3;
    if (s.nextBossMilestone > s.currentLevel + 5000)
        s.nextBossMilestone = std::max(3, s.currentLevel);

    auto fixNonNegFinite = [](double& v) {
        if (!std::isfinite(v) || v < 0.0)
            v = 0.0;
    };
    fixNonNegFinite(s.credits);
    fixNonNegFinite(s.ore);
    fixNonNegFinite(s.crystals);
    fixNonNegFinite(s.totalCredits);
    fixNonNegFinite(s.totalOre);
    fixNonNegFinite(s.oreThisLevel);
    fixNonNegFinite(s.bossCrystalPopup);
    for (double& t : s.orePerTier)
        fixNonNegFinite(t);

    if (s.prestigeCount < 0)
        s.prestigeCount = 0;

    if (s.isBonusZone) {
        const int br = static_cast<int>(s.bonusZoneRarity);
        if (br < 0 || br > static_cast<int>(OreRarity::LEGENDARY)) {
            s.isBonusZone     = false;
            s.bonusZoneRarity = OreRarity::COMMON;
        }
    }
}

} // namespace

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
    orePerTier.fill(0.0);
    if (ver >= 15) {
        f.read(reinterpret_cast<char*>(orePerTier.data()),
               orePerTier.size() * sizeof(double));
    } else {
        orePerTier[static_cast<int>(OreTier::IRON)] = std::max(0.0, ore);
    }
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
    isBonusZone     = false;
    bonusZoneRarity = OreRarity::COMMON;
    if (ver >= 16) {
        uint8_t bonusB = 0;
        f.read(reinterpret_cast<char*>(&bonusB), sizeof(bonusB));
        uint8_t bonusR = 0;
        f.read(reinterpret_cast<char*>(&bonusR), sizeof(bonusR));
        isBonusZone = (bonusB != 0);
        if (bonusR <= static_cast<uint8_t>(OreRarity::LEGENDARY))
            bonusZoneRarity = static_cast<OreRarity>(bonusR);
    }
    sanitizeLoadedState(*this);
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
    orePerTier.fill(0.0);
    oreThisLevel = 0.0;
    currentLevel = 1;
    lives        = maxLives();
    isBonusZone     = false;
    bonusZoneRarity = OreRarity::COMMON;
    // upgrades blijven staan
}
