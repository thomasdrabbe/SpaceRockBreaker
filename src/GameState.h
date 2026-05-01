#pragma once
#include <array>
#include <string>
#include <cstdint>
#include "Constants.h"

// ─────────────────────────────────────────────────────────────
//  Upgrade definition
// ─────────────────────────────────────────────────────────────
struct UpgradeDef {
    std::string name;
    std::string description;
    double      baseCost;
    double      costMult;
    int         maxLevel;   // 0 = infinite
};

// ─────────────────────────────────────────────────────────────
//  Prestige upgrade definition
// ─────────────────────────────────────────────────────────────
struct PrestigeUpgradeDef {
    std::string name;
    std::string description;
    double      baseCost;
    double      costMult;
    int         maxLevel;
};

// ─────────────────────────────────────────────────────────────
//  Chest upgrade (keys, permanent)
// ─────────────────────────────────────────────────────────────
struct ChestDef {
    std::string name;
    std::string description;
    int         baseKeyCost;
    double      keyCostMult;
    int         maxLevel;
};

// ─────────────────────────────────────────────────────────────
//  GameState
// ─────────────────────────────────────────────────────────────
class GameState {
public:
    // ── Level progress ────────────────────────────────────
    double oreThisLevel  = 0.0;   // ore verzameld in huidig level
    bool warpDriveUnlocked() const;
    bool canWarp()           const;   // warpDrive + oreThisLevel >= 10
    void doWarp();                    // level++ + reset
    // ── Currencies ────────────────────────────────────────
    double credits       = 0.0;
    double ore           = 0.0;
    double crystals      = 0.0;
    double totalCredits  = 0.0;
    double totalOre      = 0.0;
    int    prestigeCount = 0;
    int    keys          = 0;   // sleutels (blijven bij game over; voor chests)

    // ── Level / zone ──────────────────────────────────────
    int currentLevel = 1;   // advances via step E (fly to next zone)

    // ── Boss milestones (eerste bij zone 3, daarna 5, 10, 15…)
    int     nextBossMilestone = 3;
    double  bossCrystalPopup  = 0.0;

    void registerBossDefeated();

    // ── New computed stats ────────────────────────────────
    OreTier maxOreTier()    const;   // highest unlocked ore tier
    double lastOreValue = 1.0;   // waarde van meest recent gecollecte ore
    float   levelHpMult()   const;   // asteroid HP scale per level
    int     levelSpawnBonus() const; // extra asteroids per level
    std::string levelLabel() const;  // "Zone 1", "Zone 2" …
    int oreWarpRequirement() const;  // ores nodig voor warp in huidig level

        // ── Lives ─────────────────────────────────────────────
    int lives = 3;
    static constexpr int MAX_LIVES = 3;

    void loseLife();    // -1 leven, bij 0 → game over
    bool isGameOver() const { return lives <= 0; }
    void gameOver();    // reset level + ore + credits, behoud upgrades

    // ── Regular upgrade levels ────────────────────────────
    std::array<int, static_cast<int>(UpgradeID::UPGRADE_COUNT)>
        upgradeLevels{};

    // ── Prestige upgrade levels ───────────────────────────
    std::array<int, static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT)>
        prestigeLevels{};

    // ── Chest levels (keys; blijven bij prestige & game over)
    std::array<int, static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT)>
        chestLevels{};

    // ── Static catalogs ───────────────────────────────────
    static const std::array<UpgradeDef,
        static_cast<int>(UpgradeID::UPGRADE_COUNT)>        upgradeCatalog;

    static const std::array<PrestigeUpgradeDef,
        static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT)>
        prestigeCatalog;

    static const std::array<ChestDef,
        static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT)> chestCatalog;

    // ── Computed stats ────────────────────────────────────
    float gunDamage()         const;
    float fireRatePerSec()    const;
    int   turretCount()       const;
    float critChance()        const;
    float critMult()          const;
    int   splitShot()         const;

    float oreValueMult()      const;
    float autoCollectRadius() const;
    float oreLuckBonus()      const;

    int   plinkoRows()        const;
    float plinkoMultBonus()   const;
    int   maxPlinkoBalls()    const;
    float plinkoLuck()        const;

    float creditMult()        const;
    int   bulkProcess()       const;
    bool  autoPlinkoEnabled() const;

    float crystalAmp()        const;

    // ── Chest (Plinko pegs / combat / mining) ────────────
    /// Golden Pegs: aantal peg-upgrade rolls (= level × 3).
    int   chestPegUpgradeCount() const;
    float chestPlinkoBounceMult()    const;
    float chestGunFlatBonus()        const;
    float chestOreValueMult()        const;

    int   keyCostOf(ChestUpgradeID id) const;
    bool  canBuyChest(ChestUpgradeID id) const;
    void  buyChest(ChestUpgradeID id);
    int   levelOfChest(ChestUpgradeID id) const;

    // ── Upgrade helpers ───────────────────────────────────
    double costOf(UpgradeID id)         const;
    double costOf(PrestigeUpgradeID id) const;
    bool   canBuy(UpgradeID id)         const;
    bool   canBuy(PrestigeUpgradeID id) const;
    void   buy(UpgradeID id);
    void   buy(PrestigeUpgradeID id);
    int    levelOf(UpgradeID id)         const;
    int    levelOf(PrestigeUpgradeID id) const;

    // ── Prestige ──────────────────────────────────────────
    double crystalsOnPrestige() const;
    void   doPrestige();

    // ── Save / Load ───────────────────────────────────────
    bool save(const std::string& path) const;
    bool load(const std::string& path);
    /// Leest zone + credits zonder volledige game te laden (save-slot preview).
    static bool peekSaveSlot(const std::string& path,
                             int&         outZone,
                             double&      outCredits);
    void reset();

private:
    float _crystalDamageBonus()  const;
    float _crystalMiningBonus()  const;
    float _crystalEconomyBonus() const;
    float _crystalPlinkoBonus()  const;
};
