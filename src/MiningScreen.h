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

class IAudioBus;

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
                std::array<double, ORE_TIER_COUNT>& oreByTierEarned,
                float        warpChargeStars);

    void draw(sf::RenderTarget& target,
              const GameState& state,
              float            warpCharge,
              float            warpFlashRemain,
              float            animTime) const;

    bool trySpawnKeyAsteroid(GameState& state);
    void spawnBonusZoneKeys(const GameState& state);
    void setKeyAsteroidsEnabled(bool enabled) {
        m_asteroids.setKeyAsteroidsEnabled(enabled);
    }
    bool trySpawnBoss(GameState& state);
    int  pullPendingKeyDrop();

    void syncTurrets(const GameState& state);

    void collectAllOre(double& oreOut,
                       std::array<double, ORE_TIER_COUNT>& oreByTierOut,
                       GameState& state);
    void clearAll();
    void prepareNewRun();

    bool pullBossReturnToBase();
    /// Boss verslagen maar nog loot op het veld — mining moet door simuleren.
    bool bossReturnPending() const { return m_pendingBossReturnToBase; }

    bool hasLivingBoss() const;

    /// Meteor-timer + shower-queue starten. Wordt vanuit Game aangeroepen.
    void tickMeteorShower(float dt, GameState& state, float asteroidHpMult);
    /// Alleen meteoren bewegen (Game roept dit aan bij gepauzeerde mining-tab).
    void advanceMeteorsOnly(float dt);
    void tickMeteorSpawnQueue();
    void setAudioBus(IAudioBus* audioBus);

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

    // ── Nebula (procedurele wolken achter sterren) ─────────
    struct NebulaCloud {
        sf::Vector2f basePos{};
        float        radiusX   = 1.f;
        float        radiusY   = 1.f;
        float        rotation  = 0.f; // graden
        float        alpha     = 28.f; // basis 15–40 (wolken) / hoger voor highlights
        float        driftX    = 0.f;
        float        driftY    = 0.f;
        float        phase     = 0.f;
    };
    static constexpr int NEBULA_CLOUD_COUNT      = 20;
    static constexpr int NEBULA_HIGHLIGHT_COUNT  = 8;
    std::array<NebulaCloud, NEBULA_CLOUD_COUNT>     m_nebulaClouds{};
    std::array<NebulaCloud, NEBULA_HIGHLIGHT_COUNT> m_nebulaHighlights{};
    int       m_lastNebulaZone   = -1;
    bool      m_lastNebulaBonus  = false;
    OreRarity m_lastNebulaRarity = OreRarity::COMMON;
    int       m_nebulaCachedIw = -1;
    int       m_nebulaCachedIh = -1;

    /// PNG-volgorde moet gelijk blijven aan `nebulaPngIndex()`.
    static constexpr int NEBULA_PNG_COUNT = 9;
    std::array<sf::Texture, NEBULA_PNG_COUNT> m_nebulaPngTex{};

    void loadNebulaPngTextures();
    static int nebulaPngIndex(int zone, bool isBonusZone, OreRarity bonusRarity);

    static constexpr int ASTEROID_TEX_COUNT = 6;
    std::array<sf::Texture, ASTEROID_TEX_COUNT> m_asteroidTextures{};
    bool                                           m_asteroidTexturesLoaded = false;
    void                                           loadAsteroidTextures();
    void                                           loadAsteroidGlowShader();
    void drawAsteroidsWithSprites(sf::RenderTarget& target,
                                  float             animTime) const;

    /// Additieve PNG-silhouet-gloed; `mutable` voor `setUniform` vanuit `const` draw.
    mutable sf::Shader m_asteroidGlowShader{};
    mutable bool       m_asteroidGlowShaderReady = false;

    void buildNebulaClouds(int zone, bool isBonusZone, OreRarity bonusRarity);
    void drawNebulaTexture(sf::RenderTarget& target, const GameState& state) const;
    void drawNebula(sf::RenderTarget& target,
                    float            animTime,
                    int              zone,
                    bool             isBonusZone,
                    OreRarity        bonusRarity) const;
    static sf::Color nebulaColorForZone(int         zone,
                                        bool         isBonusZone,
                                        OreRarity    bonusRarity,
                                        bool         secondary);
    static float nebulaIntensityForZone(int         zone,
                                        bool         isBonusZone,
                                        OreRarity    bonusRarity);

    // ── Collector volgt speler ────────────────────────────
    sf::Vector2f m_collectorPos;

    // ── State ─────────────────────────────────────────────
    int m_lastTurretCnt = 0;
    int m_pendingKeyDrop = 0;
    bool m_pendingBossReturnToBase = false;
    IAudioBus* m_audio = nullptr;

    float                  m_meteorTimeToNext = -1.f;

    void resetMeteorShowerSchedule();

    // ── Collision ─────────────────────────────────────────
    void resolveCollisions(GameState& state);

    // ── Draw helpers ──────────────────────────────────────
    void drawStarfield(sf::RenderTarget& target, float warpCharge,
                       const GameState& state) const;
    void drawZoneBackground(sf::RenderTarget& target,
                             const GameState&  state,
                             float             animTime) const;
    void drawWarpFlashOverlay(sf::RenderTarget& target,
                               float            warpFlashRemain) const;
    void drawCollector  (sf::RenderTarget& target) const;
    void drawCollectRing(sf::RenderTarget& target,
                         const GameState&  state)  const;
    void drawHUD(sf::RenderTarget& target, const GameState&  state,
    float             warpCharge) const;


    static int targetAsteroidCount(int turrets);
};
