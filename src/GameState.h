#pragma once
#include <array>
#include <string>
#include <cstdint>
#include "Constants.h"

struct UpgradeNodeDef;

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
    int         maxLevel;   // 0 = oneindig
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
    void doWarp();                    // normale warp / bonus-zone logica
    float warpDurationSec() const;

    /// Optionele bonus-zone (zelfde `currentLevel`, extra loot).
    bool        isBonusZone     = false;
    OreRarity   bonusZoneRarity = OreRarity::COMMON;

    OreTier bonusZoneMinOreTier() const;
    float   bonusZoneOreValueMult() const;
    /// Sleutel-asteroïde ore-tier: `preferred` maar nooit boven `maxUnlocked`.
    static OreTier clampKeyOreTier(OreTier preferred, OreTier maxUnlocked);

    std::string zoneNameFor(int zone) const;
    std::string currentZoneName() const;
    // ── Currencies ────────────────────────────────────────
    double credits       = 0.0;
    double ore           = 0.0;
    double crystals      = 0.0;
    double totalCredits  = 0.0;
    double totalOre      = 0.0;
    std::array<double, ORE_TIER_COUNT> orePerTier{};
    int    prestigeCount = 0;
    int    keys          = 0;   // sleutels (blijven bij game over; voor chests)
    void addCredits(double amount);
    bool spendCredits(double amount);
    void addOre(double amount, bool countForWarp = true);
    bool spendOre(double amount);
    void addOreTiered(const std::array<double, ORE_TIER_COUNT>& oreByTier,
                      bool countForWarp = true);
    void addCrystals(double amount);
    void addKeys(int amount);
    bool consumeKeys(int amount = 1);
    OreTier dominantOreTier() const;
    sf::Color dominantOreColor() const;
    bool deductOneOre(OreTier& outTier);
    bool spendOreForPlinko(double amount, OreTier& outTier);

    /// Progressieve unlocks (index 1…8 gebruikt; 0 ongebruikt).
    std::array<bool, 9> unlockPhaseDone{};
    bool                keyAsteroidsEnabled = false;
    /// Alle meteoren van één shower met torret-kogels vernietigd → koop Meteor-upgrades.
    bool                meteorDestroyerUnlocked = false;

    Difficulty difficulty = Difficulty::Medium;

    int   maxLives()              const;
    float difficultyAsteroidHpMult() const;
    float hitInvulnerabilitySec() const;

    /// Makkelijk: mining stopt op andere tabs (behalve boss-terug-flow).
    bool miningPausesWhenOffMiningTab() const;
    /// Normaal: knop om tijdens een run vanuit shop/plinko/chests naar basis te gaan.
    bool showsRetreatToBaseOnOtherTabs() const;

    float meteorShowerIntervalSec() const;
    int   meteorShowerMeteorCount() const;

    /// 1 key → willekeurige chest-upgrade. Geen keys → false.
    bool openOneChest(ChestUpgradeID* outChosen = nullptr);

    // ── Level / zone ──────────────────────────────────────
    int currentLevel = 1;   // advances via warp tijdens run

    /// Hoogste zone ooit bereikt (warp); bepaalt start-zone knoppen in basis.
    int highestZoneReached = 1;

    // ── Boss milestones (zone 5, 10, 15, …)
    int     nextBossMilestone = FIRST_BOSS_ZONE;
    double  bossCrystalPopup  = 0.0;

    void registerBossDefeated();
    void registerZoneReached(int zone);
    bool isZoneReachable(int zone) const;
    void beginRunAtZone(int startZone);
    void endRunRestoreZone();

    bool autoPlinkoUnlockedByBoss() const {
        return prestigeCount > 0 || nextBossMilestone > FIRST_BOSS_ZONE;
    }

    bool pendingAutoPlinkoBossNotif = false;

    // ── New computed stats ────────────────────────────────
    OreTier maxOreTier()    const;   // highest unlocked ore tier
    double lastOreValue = 1.0;   // waarde van meest recent gecollecte ore
    float   levelHpMult()   const;   // asteroid HP scale per level
    int     levelSpawnBonus() const; // extra asteroids per level
    std::string levelLabel() const;  // "Zone N — …" met procedurele naam
    int oreWarpRequirement() const;  // ores nodig voor warp in huidig level

        // ── Lives ─────────────────────────────────────────────
    int lives = 3;
    /// Historisch maximum voor UI; werkelijk max = maxLives() (easy/hard).
    static constexpr int MAX_LIVES = 4;

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
    /// Basis 2.8s; stijgt met Bullet Range-upgrade (kogels vliegen verder).
    float bulletLifetimeSec() const;

    float maxFuel()         const;
    float fuelPassiveDrain() const;
    float fuelMoveDrain()   const;
    float fuelShootDrain()  const;
    float fuelOnKill()      const;
    float fuelOnPickup()    const;
    float warpFuelRefillChance() const;
    float fuelTurretDrain() const;

    float oreValueMult()      const;
    float autoCollectRadius() const;
    float oreLuckBonus()      const;

    int   plinkoRows()        const;
    float plinkoMultBonus()   const;
    int   maxPlinkoBalls()    const;
    float plinkoLuck()        const;
    /// Ore per Plinko-bal; lichte stijging bij zeer hoge voorraad (sink).
    double plinkoBallOreCost() const;

    float creditMult()        const;
    int   bulkProcess()       const;
    bool  autoPlinkoEnabled() const;
    /// Ballen per auto-interval (1 bij Lv1, +1 per extra level).
    int   autoPlinkoBallsPerTick() const;

    float crystalAmp()        const;

    // ── Chest (Plinko pegs / combat / mining) ────────────
    /// Golden Pegs: aantal peg-upgrade rolls (= level × 3).
    int   chestPegUpgradeCount() const;
    /// Duplicator pegs: rolls zoals golden pegs (level x 3).
    int   chestDuplicatorRollCount() const;
    /// 1.0 zonder Trough Boost; elk niveau ×1.32 extra (stapelt op valbak-mults).
    float chestPlinkoSlotMult() const;

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

    bool isNodeUnlocked(const UpgradeNodeDef& node) const;
    bool isNodeVisible(const UpgradeNodeDef& node)  const;

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

    void migrateUnlockProgressFromLegacyState();

    /// Ore per tier: caps + conversie naar hogere tier (1× per seconde vanuit Game).
    void processOreFusion();

private:
    int levelBeforeRun = 0;

    OreRarity rollBonusZoneRarity();

    float _crystalDamageBonus()  const;
    float _crystalMiningBonus()  const;
    float _crystalEconomyBonus() const;
    float _crystalPlinkoBonus()  const;
};
