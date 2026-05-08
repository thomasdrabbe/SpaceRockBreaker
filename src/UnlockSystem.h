#pragma once
#include <string>
#include "Constants.h"
#include "Shop.h"

class GameState;
class NotificationSystem;

enum class UnlockPhase : int {
    NONE = 0,
    PLINKO_TAB = 1,
    SHOP_TAB = 2,
    WEAPONS = 3,
    MINING = 4,
    AUTO_PLINKO = 5,
    CHESTS = 6,
    ECONOMY = 7,
    PRESTIGE = 8,
};

class IUnlockEffects {
public:
    virtual ~IUnlockEffects() = default;
    virtual void setTabVisible(Tab tab, bool visible) = 0;
    virtual void setShopCategoryVisible(ShopCategory category, bool visible) = 0;
    virtual void setShopMiningWarpOnly(bool warpOnly) = 0;
    virtual void setShopPlinkoAutoOnly(bool autoOnly) = 0;
    virtual void focusShopCategory(ShopCategory category) = 0;
    virtual void setKeyAsteroidsEnabled(bool enabled) = 0;
};

/// Voortgang naar de eerstvolgende unlock-fase (Plinko-tab hintpaneel).
struct UnlockNextHint {
    bool        allPhasesComplete = false;
    std::string heading;
    std::string phaseName;
    std::string progressDetail;
    float       progress01 = 0.f;
};

[[nodiscard]] UnlockNextHint computeUnlockNextHint(const GameState& state);

class UnlockSystem {
public:
    void update(GameState&           state,
                NotificationSystem&  notifications,
                IUnlockEffects&      effects);
};
