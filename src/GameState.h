#pragma once
#include <array>
#include <string>
#include <cstdint>
#include "Constants.h"

// ─────────────────────────────────────────────────────────────
//  Upgrade definition  (static data per upgrade type)
// ─────────────────────────────────────────────────────────────
struct UpgradeDef {
    std::string name;
    std::string description;   // uses {val} as placeholder for current effect
    double      baseCost;
    double      costMult;      // cost = baseCost * costMult ^ level
    int         maxLevel;      // 0 = infinite
};

// ─────────────────────────────────────────────────────────────
//  Prestige upgrade definition
// ─────────────────────────────────────────────────────────────
struct PrestigeUpgradeDef {
    std::string name;
    std::string description;
    double      baseCost;      // cost in crystals
    double      costMult;
    int         maxLevel;      // 0 = infinite
};

// ─────────────────────────────────────────────────────────────
//  GameState  — single source of truth for all runtime data
// ─────────────────────────────────────────────────────────────
class GameState {
public:
    // ── Currencies ────────────────────────────────────────
    double credits        = 0.0;
    double ore            = 0.0;
    double crystals       = 0.0;

    double totalCredits   = 0.0;   // lifetime (for prestige calc)
    double totalOre       = 0.0;
    int    prestigeCount  = 0;

    // ── Regular upgrade levels (reset on prestige) ────────
    std::array<int, static_cast<int>(UpgradeID::UPGRADE_COUNT)>
        upgradeLevels{};

    // ── Prestige upgrade levels (permanent) ───────────────
    std::array<int, static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT)>
        prestigeLevels{};

    // ── Static catalogs (filled in .cpp) ──────────────────
    static const std::array<UpgradeDef,
        static_cast<int>(UpgradeID::UPGRADE_COUNT)>        upgradeCatalog;

    static const std::array<PrestigeUpgradeDef,
        static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT)> prestigeCatalog;

    // ── Computed stats ────────────────────────────────────
    float gunDamage()          const;
    float fireRatePerSec()     const;
    int   turretCount()        const;
    float critChance()         const;
    float critMult()           const;
    int   splitShot()          const;

    float oreValueMult()       const;
    float autoCollectRadius()  const;
    float oreLuckBonus()       const;

    int   plinkoRows()         const;
    float plinkoMultBonus()    const;
    int   maxPlinkoBalls()     const;
    float plinkoLuck()         const;

    float creditMult()         const;
    int   bulkProcess()        const;
    bool  autoPlinkoEnabled()  const;

    // ── Crystal amplifier (affects all regular upgrades) ──
    float crystalAmp()         const;  // combined prestige bonus

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
    double crystalsOnPrestige()  const;   // preview
    void   doPrestige();

    // ── Save / Load ───────────────────────────────────────
    bool save(const std::string& path) const;
    bool load(const std::string& path);
    void reset();

private:
    float _crystalDamageBonus()   const;
    float _crystalMiningBonus()   const;
    float _crystalEconomyBonus()  const;
    float _crystalPlinkoBonus()   const;
};