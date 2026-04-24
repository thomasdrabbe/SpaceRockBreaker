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
//  GameState
// ─────────────────────────────────────────────────────────────
class GameState {
public:
    // ── Currencies ────────────────────────────────────────
    double credits       = 0.0;
    double ore           = 0.0;
    double crystals      = 0.0;
    double totalCredits  = 0.0;
    double totalOre      = 0.0;
    int    prestigeCount = 0;

    // ── Regular upgrade levels ────────────────────────────
    std::array<int, static_cast<int>(UpgradeID::UPGRADE_COUNT)>
        upgradeLevels{};

    // ── Prestige upgrade levels ───────────────────────────
    std::array<int, static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT)>
        prestigeLevels{};

    // ── Static catalogs ───────────────────────────────────
    static const std::array<UpgradeDef,
        static_cast<int>(UpgradeID::UPGRADE_COUNT)>        upgradeCatalog;

    static const std::array<PrestigeUpgradeDef,
        static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT)>
        prestigeCatalog;

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
    void reset();

private:
    float _crystalDamageBonus()  const;
    float _crystalMiningBonus()  const;
    float _crystalEconomyBonus() const;
    float _crystalPlinkoBonus()  const;
};
