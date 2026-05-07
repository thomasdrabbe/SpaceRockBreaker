#pragma once
#include <string>

class GameState;
class NotificationSystem;
class Game;
class Shop;
class MiningScreen;

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
                Game&                game,
                Shop&                shop,
                MiningScreen&        mining);
};
