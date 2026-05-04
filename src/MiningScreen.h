#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Constants.h"
#include "GameState.h"
#include "Asteroid.h"
#include "Bullet.h"
#include "Turret.h"
#include "Ore.h"
#include "KeyPickup.h"
#include "Particle.h"
#include "Player.h"

// ─────────────────────────────────────────────────────────────
//  Star (parallax background)
// ─────────────────────────────────────────────────────────────
struct Star {
    sf::Vector2f pos;
    float        speed;
    float        radius;
    uint8_t      brightness;
};

// ─────────────────────────────────────────────────────────────
//  MiningScreen
// ─────────────────────────────────────────────────────────────
class MiningScreen {
public:
    bool playerHit() const;
    //--player pos voor emit explosion
    sf::Vector2f playerPos() const { return m_player.pos; }

    MiningScreen();

    void init(sf::Font& font,
              float panelX, float panelY,
              float panelW, float panelH,
              const sf::Texture* keyIconTex = nullptr);

    /// creditsEarned : credits verdiend via Plinko (niet hier)
    /// oreEarned     : ore opgepakt deze frame
    void update(float      dt,
                GameState& state,
                double&    creditsEarned,
                double&    oreEarned,
                float        warpChargeStars);

    void draw(sf::RenderTarget& target,
              const GameState& state,
              float            warpCharge,
              float            warpFlashRemain,
              float            animTime) const;

    bool trySpawnKeyAsteroid(GameState& state);
    bool trySpawnBoss(GameState& state);
    int  pullPendingKeyDrop();

    void syncTurrets(const GameState& state);

    void collectAllOre(double& oreOut, GameState& state);
    void clearAll();
    void prepareNewRun();

    bool pullBossReturnToBase();
    /// Boss verslagen maar nog loot op het veld — mining moet door simuleren.
    bool bossReturnPending() const { return m_pendingBossReturnToBase; }

    /// Meteor-timer + shower-queue starten. Wordt vanuit Game aangeroepen.
    void tickMeteorShower(float dt, GameState& state, float asteroidHpMult);
    /// Alleen meteoren bewegen (Game roept dit aan bij gepauzeerde mining-tab).
    void advanceMeteorsOnly(float dt);
    void tickMeteorSpawnQueue();

    // ── Sub-system toegang ────────────────────────────────
    OreManager&     ores()      { return m_ores;      }
    ParticleSystem& particles() { return m_particles; }

private:
    sf::Font*          m_font = nullptr;
    const sf::Texture* m_keyIconTex = nullptr;
    sf::Texture        m_playerShipTex;
    float        m_x    = 0.f;
    float        m_y    = 0.f;
    float        m_w    = 0.f;
    float        m_h    = 0.f;

    // ── Entities ──────────────────────────────────────────
    AsteroidManager m_asteroids;
    BulletManager   m_bullets;
    TurretManager   m_turrets;
    OreManager      m_ores;
    KeyPickupManager m_keyPickups;
    ParticleSystem  m_particles;
    Player          m_player;

    // ── Starfield ─────────────────────────────────────────
    static constexpr int STAR_COUNT = 220;
    std::array<Star, STAR_COUNT> m_stars;
    void buildStarfield();

    // ── Collector volgt speler ────────────────────────────
    sf::Vector2f m_collectorPos;

    // ── State ─────────────────────────────────────────────
    int m_lastTurretCnt = 0;
    int m_pendingKeyDrop = 0;
    bool m_pendingBossReturnToBase = false;

    float                  m_meteorTimeToNext = -1.f;

    void resetMeteorShowerSchedule();

    // ── Collision ─────────────────────────────────────────
    void resolveCollisions(GameState& state);

    // ── Draw helpers ──────────────────────────────────────
    void drawStarfield(sf::RenderTarget& target, float warpCharge) const;
    void drawWarpFlashOverlay(sf::RenderTarget& target,
                               float            warpFlashRemain) const;
    void drawCollector  (sf::RenderTarget& target) const;
    void drawCollectRing(sf::RenderTarget& target,
                         const GameState&  state)  const;
    void drawHUD(sf::RenderTarget& target, const GameState&  state,
    float             warpCharge) const;


    static int targetAsteroidCount(int turrets);
};
