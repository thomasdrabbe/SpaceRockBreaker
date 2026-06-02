#pragma once
#include <algorithm>
#include <array>
#include <string>
#include <SFML/Graphics/Color.hpp>

// ─── Window ───────────────────────────────────────────────────
constexpr unsigned WINDOW_WIDTH  = 2560;
constexpr unsigned WINDOW_HEIGHT = 1440;
constexpr int      TARGET_FPS    = 60;
const std::string  WINDOW_TITLE  = "Space Rock Breaker";

// ─── Tabs ─────────────────────────────────────────────────────
enum class Tab { MINING = 0, PLINKO, SKILL_TREE, CHESTS, PRESTIGE };
constexpr int TAB_COUNT = 5;

// ─── Chest upgrades (keys, permanent; blijven bij prestige) ──
enum class ChestUpgradeID {
    PLINKO_PEG_SIZE = 0,
    PLINKO_SLOT_MULT,
    PLINKO_DUPLICATOR_PEG,
    PLINKO_MULT_CHEST,
    PLINKO_LUCK_CHEST,
    PLINKO_REFINER_PEG,
    CHEST_UPGRADE_COUNT
};

/// Mining auto-aim modus (TARGET_PRIORITY upgrade).
enum class TargetMode : std::uint8_t {
    NEAREST   = 0,
    BOSS      = 1,
    KEY       = 2,
    ORE_TIER  = 3,
    LOWEST_HP = 4,
    TARGET_MODE_COUNT
};

// ─── Upgrade IDs (reset on prestige) ──────────────────────────
enum class UpgradeID {
    // Weapons
    GUN_DAMAGE = 0,
    FIRE_RATE,
    TURRET_COUNT,
    CRIT_CHANCE,
    CRIT_MULT,
    SPLIT_SHOT,
    // Mining
    ORE_VALUE,
    AUTO_COLLECT_RADIUS,
    ORE_LUCK,
    ASTEROID_HP,
    // Plinko
    PLINKO_ROWS,
    PLINKO_MULT,
    PLINKO_BALLS,
    PLINKO_LUCK,
    // Economy
    CREDIT_MULT,
    BULK_PROCESS,
    AUTO_PLINKO,
    // Ore tier unlocks (maxLevel = 1 each; overige upgrades hebben caps in catalog)
    UNLOCK_BRONZE,
    UNLOCK_SILVER,
    UNLOCK_GOLD,
    UNLOCK_DIAMOND,
    UNLOCK_PLATINUM,
    UNLOCK_TITANIUM,
    UNLOCK_IRIDIUM,
    WARP_DRIVE,
    BULLET_RANGE,
    /// Na easter egg (hele meteor-shower vernietigen): asteroïden raken.
    METEOR_DAMAGE,
    METEOR_SIZE,
    FUEL_CAPACITY,
    FUEL_EFFICIENCY,
    FUEL_ON_KILL,
    FUEL_ON_PICKUP,
    FUEL_WARP_REFILL,
    TURRET_DAMAGE,
    TURRET_FIRE_RATE,
    TURRET_FUEL_DRAIN,
    ORE_ON_KILL,
    SHIP_SPEED,
    SPEED_EFFICIENCY,
    TARGET_PRIORITY,
    SEEKING_BULLETS,
    SHIELD_HP,
    SHIELD_RECHARGE,
    SHIELD_DELAY,
    SHIELD_MULTI_HIT,
    WARP_ORE_BONUS,
    AUTO_WARP,
    EXPLOSIVE_ASTEROIDS,
    CHAIN_REACTION,
    AUTO_SELL_THRESHOLD,
    SATELLITE,
    UPGRADE_COUNT   // sentinel

};

// ─── Prestige Upgrade IDs (permanent) ─────────────────────────
enum class PrestigeUpgradeID {
    CRYSTAL_DAMAGE = 0,
    CRYSTAL_MINING,
    CRYSTAL_ECONOMY,
    CRYSTAL_PLINKO,
    CRYSTAL_RETENTION,
    GEM_VAULT,
    PRESTIGE_UPGRADE_COUNT
};

// ─── Gems (cap-break + crafting) ─────────────────────────────
enum class GemType : std::uint8_t {
    RUBY = 0,
    SAPPHIRE,
    EMERALD,
    TOPAZ,
    AMETHYST,
    AQUAMARINE,
    DIAMOND,
    OBSIDIAN,
    GEM_TYPE_COUNT
};

constexpr int GEM_TYPE_COUNT_INT = static_cast<int>(GemType::GEM_TYPE_COUNT);

struct GemDef {
    const char* name;
    const char* category;
    int         unlockZone;
    sf::Color   color;
    sf::Color   glowColor;
    const char* spritePath;
};

inline constexpr GemDef GEM_DEFS[GEM_TYPE_COUNT_INT] = {
    { "Ruby",       "Weapons",   1,  sf::Color(220,  40,  60), sf::Color(255, 120, 140),
      "assets/gems/gem_ruby.png" },
    { "Sapphire",   "Fuel",      5,  sf::Color( 50, 100, 220), sf::Color(100, 160, 255),
      "assets/gems/gem_sapphire.png" },
    { "Emerald",    "Mining",   10,  sf::Color( 40, 180,  80), sf::Color(100, 240, 140),
      "assets/gems/gem_emerald.png" },
    { "Topaz",      "Economy",  15,  sf::Color(220, 140,  30), sf::Color(255, 200,  80),
      "assets/gems/gem_topaz.png" },
    { "Amethyst",   "Plinko",   20,  sf::Color(150,  50, 220), sf::Color(210, 130, 255),
      "assets/gems/gem_amethyst.png" },
    { "Aquamarine", "Warp",     30,  sf::Color( 50, 200, 190), sf::Color(120, 240, 230),
      "assets/gems/gem_aquamarine.png" },
    { "Diamond",    "Universal", 50, sf::Color(210, 240, 255), sf::Color(255, 255, 255),
      "assets/gems/gem_diamond.png" },
    { "Obsidian",   "Prestige", 75, sf::Color( 40,  20,  60), sf::Color(140,  60, 255),
      nullptr },
};

// ─── Asteroid tiers (size/hp) ─────────────────────────────────
enum class AsteroidTier { SMALL = 0, MEDIUM, LARGE, GIANT };

// ─── Ore Rarity ───────────────────────────────────────────────
enum class OreRarity {
    COMMON = 0,
    UNCOMMON,
    RARE,
    EPIC,
    MYTHIC,
    LEGENDARY,
    RARITY_COUNT
};

// ─── Ore tiers (material) ─────────────────────────────────────
enum class OreTier {
    IRON = 0,
    BRONZE,
    SILVER,
    GOLD,
    DIAMOND,
    PLATINUM,
    TITANIUM,
    IRIDIUM,
    ORE_TIER_COUNT
};

constexpr int ORE_TIER_COUNT = static_cast<int>(OreTier::ORE_TIER_COUNT);

inline double oreTierBaseValue(OreTier tier) {
    constexpr std::array<double, ORE_TIER_COUNT> kBaseValues = {
        1.0, 5.0, 30.0, 200.0, 1500.0, 12000.0, 100000.0, 1000000.0
    };
    const int ti = std::clamp(static_cast<int>(tier), 0, ORE_TIER_COUNT - 1);
    return kBaseValues[static_cast<std::size_t>(ti)];
}

inline sf::Color oreTierColor(OreTier tier) {
    constexpr std::array<sf::Color, ORE_TIER_COUNT> kTierColors = {
        sf::Color(140, 140, 150), // IRON
        sf::Color(200, 120, 50),  // BRONZE
        sf::Color(210, 215, 225), // SILVER
        sf::Color(255, 215, 50),  // GOLD
        sf::Color(140, 230, 255), // DIAMOND
        sf::Color(160, 185, 255), // PLATINUM
        sf::Color(90, 120, 220),  // TITANIUM
        sf::Color(160, 60, 220),  // IRIDIUM
    };
    const int ti = std::clamp(static_cast<int>(tier), 0, ORE_TIER_COUNT - 1);
    return kTierColors[static_cast<std::size_t>(ti)];
}

// ─── Entity limits ────────────────────────────────────────────
constexpr int MAX_ASTEROIDS    = 80;
/// Vrijhouden in de pool voor meteorregen (max 14) + key/boss overlap.
constexpr int ASTEROID_POOL_EVENT_HEADROOM = 20;
constexpr int MAX_BULLETS      = 300;
constexpr int MAX_ORE          = 1000;
constexpr int MAX_KEY_PICKUPS  = 64;
constexpr int MAX_GEM_PICKUPS  = 4;
constexpr int MAX_PARTICLES    = 600;
constexpr int MAX_PLINKO_BALLS = 200;

// ─── Plinko board geometry ────────────────────────────────────
constexpr int   PLINKO_MIN_ROWS    = 16;
constexpr int   PLINKO_MAX_ROWS    = 16;
constexpr float PLINKO_PEG_RADIUS  = 6.f;
constexpr float PLINKO_BALL_RADIUS = 9.f;
constexpr float PLINKO_GRAVITY     = 600.f;

// ─── Layout ───────────────────────────────────────────────────
constexpr float SIDE_PANEL_W = 240.f;
constexpr float TAB_BAR_H    = 46.f;

// ─── Save ─────────────────────────────────────────────────────
const std::string SAVE_FILE = "srb_save.bin"; // legacy (wordt naar slot 0 gemigreerd)
constexpr int     SAVE_SLOT_COUNT = 3;
constexpr int     SAVE_VERSION    = 24;
/// `upgradeLevels` in saves vóór volledige upgrade-roadmap batch 2.
constexpr int     LEGACY_UPGRADE_SAVE_COUNT_V22 = 37;
/// `upgradeLevels` in saves vóór TURRET_* / ORE_ON_KILL.
constexpr int     LEGACY_UPGRADE_SAVE_COUNT_V21 = 34;
/// `upgradeLevels` in saves vóór FUEL_ON_PICKUP / FUEL_WARP_REFILL.
constexpr int     LEGACY_UPGRADE_SAVE_COUNT_V20 = 32;
/// Eerste zone-boss en daarna elke N zones (5, 10, 15, …).
constexpr int     FIRST_BOSS_ZONE       = 5;
constexpr int     BOSS_ZONE_INTERVAL    = 5;
/// Auto-Plinko (gratis tier + koopbaar) pas vanaf 2e boss-zone (niet na 1e boss).
constexpr int     AUTO_PLINKO_UNLOCK_ZONE = FIRST_BOSS_ZONE + BOSS_ZONE_INTERVAL;
/// Gratis boss-auto na 1e boss: zelfde 1 bal, maar dit veelvoud op fire-interval.
constexpr float   AUTO_PLINKO_HALF_INTERVAL_MULT = 2.f;
/// BAL4: max skill-kopen per hub-bezoek (voor prestige +3).
constexpr int     HUB_UPGRADE_BUY_LIMIT_BASE       = 10;
constexpr int     HUB_UPGRADE_BUY_LIMIT_PER_PRESTIGE = 3;
/// BAL3: upgrade-prijzen stijgen licht per verslagen boss.
constexpr double  BAL3_UPGRADE_COST_PER_BOSS = 0.06;
/// Zone-knoppen per rij op het basis-paneel (meerdere rijen vanaf zone 6+).
constexpr int     START_ZONE_BUTTONS_PER_ROW = 8;
/// Max zones in start-kiezer (voorkomt freeze bij corrupte save).
constexpr int     START_ZONE_PICKER_MAX_ZONES = 50;
/// Aantal `upgradeLevels`-entries in saves vóór METEOR_DAMAGE / METEOR_SIZE.
constexpr int     LEGACY_UPGRADE_SAVE_COUNT_V16 = 26;

// ─── Nieuwe spel-moeilijkheid ─────────────────────────────────
enum class Difficulty {
    Easy = 0,
    Medium,
    Hard,
};

inline std::string saveSlotPath(int slot) {
    return "srb_save_" + std::to_string(slot) + ".bin";
}

// ─── Run (mining) vs basis (hub) ────────────────────────────
enum class RunMode { BASE, RUNNING };

/// Waarom een mining-run eindigt (terug naar basis).
enum class RunEndReason : std::uint8_t {
    NONE = 0,
    FUEL_EMPTY,
    ASTEROID_HIT,
};

// ─── Key asteroid ───────────────────────────────────────────
constexpr float KEY_ASTEROID_SPAWN_DELAY_SEC = 30.f;

// ─── Warp hold (mining): star streak + witte flits ──────────
/// Warp-laadduur: `GameState::warpDurationSec()` (afhankelijk van Warp Drive-level).
constexpr float WARP_FLASH_DURATION_SEC  = 0.44f;
/// Ster-streak tijdens warp: we = warpCharge^POW; totale factor × SCALE.
constexpr float WARP_STAR_STREAK_POW       = 2.35f;
constexpr float WARP_STAR_STREAK_LINEAR  = 22.f;
constexpr float WARP_STAR_STREAK_QUAD    = 110.f;
constexpr float WARP_STAR_STREAK_SCALE     = 2.6f;
