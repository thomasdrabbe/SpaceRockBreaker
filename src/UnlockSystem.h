#pragma once

class GameState;
class NotificationSystem;
class Game;
class Shop;
class MiningScreen;

class UnlockSystem {
public:
    void update(GameState&           state,
                NotificationSystem&  notifications,
                Game&                game,
                Shop&                shop,
                MiningScreen&        mining);
};
