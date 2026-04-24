#pragma once
#include <string>

// ─── Window ───────────────────────────────────────────────────
constexpr unsigned WINDOW_WIDTH  = 2560;
constexpr unsigned WINDOW_HEIGHT = 1440;
constexpr int      TARGET_FPS    = 60;
const std::string  WINDOW_TITLE  = "Space Rock Breaker";

// ─── Tabs ─────────────────────────────────────────────────────
enum class Tab { MINING = 0, PLINKO, SHOP, PRESTIGE };
constexpr int TAB_COUNT = 4;

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

// ─── Asteroid tiers ───────────────────────────────────────────
enum class AsteroidTier { SMALL = 0, MEDIUM, LARGE, GIANT };

// ─── Entity limits ────────────────────────────────────────────
constexpr int MAX_ASTEROIDS    = 40;
constexpr int MAX_BULLETS      = 300;
constexpr int MAX_ORE          = 250;
constexpr int MAX_PARTICLES    = 600;
constexpr int MAX_PLINKO_BALLS = 24;

// ─── Plinko board geometry ────────────────────────────────────
constexpr int   PLINKO_MIN_ROWS   = 8;
constexpr int   PLINKO_MAX_ROWS   = 16;
constexpr float PLINKO_PEG_RADIUS = 6.f;
constexpr float PLINKO_BALL_RADIUS = 9.f;
constexpr float PLINKO_GRAVITY    = 600.f;

// ─── Layout ───────────────────────────────────────────────────
constexpr float SIDE_PANEL_W = 240.f;
constexpr float TAB_BAR_H    = 46.f;

// ─── Save ─────────────────────────────────────────────────────
const std::string SAVE_FILE    = "srb_save.bin";
constexpr int     SAVE_VERSION = 2;
