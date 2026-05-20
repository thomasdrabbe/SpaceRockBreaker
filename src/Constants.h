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
    CHEST_UPGRADE_COUNT
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
    // Ore tier unlocks (maxLevel = 1 each)
    UNLOCK_BRONZE,
    UNLOCK_SILVER,
    UNLOCK_GOLD,
    UNLOCK_DIAMOND,
    UNLOCK_PLATINUM,
    UNLOCK_TITANIUM,
    UNLOCK_IRIDIUM,
    WARP_DRIVE,
    BULLET_RANGE,
    /// Na easter egg (alle shower-meteoren door torret-kogels): asteroïden raken.
    METEOR_DAMAGE,
    METEOR_SIZE,
    UPGRADE_COUNT   // sentinel

};

// ─── Prestige Upgrade IDs (permanent) ─────────────────────────
enum class PrestigeUpgradeID {
    CRYSTAL_DAMAGE = 0,
    CRYSTAL_MINING,
    CRYSTAL_ECONOMY,
    CRYSTAL_PLINKO,
    CRYSTAL_RETENTION,
    PRESTIGE_UPGRADE_COUNT
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
        1.0, 3.0, 8.0, 20.0, 55.0, 140.0, 380.0, 1000.0
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
constexpr int MAX_KEY_PICKUPS = 64;
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
constexpr int     SAVE_VERSION    = 18;
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
