#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Constants.h"
#include "Particle.h"

// ─────────────────────────────────────────────────────────────
//  Ore drop info
// ─────────────────────────────────────────────────────────────
struct OreDrop {
    sf::Color color;
    double    value;
    int       count;
};

// ─────────────────────────────────────────────────────────────
//  Asteroid
// ─────────────────────────────────────────────────────────────
class Asteroid {
public:
    sf::Vector2f  pos;
    sf::Vector2f  vel;
    float         rotation     = 0.f;
    float         rotationRate = 0.f;
    float         radius       = 0.f;
    float         hp           = 0.f;
    float         maxHp        = 0.f;
    AsteroidTier  tier         = AsteroidTier::SMALL;
    bool          alive        = false;
    sf::Color     color;
    sf::ConvexShape shape;
    OreDrop       oreDrop;
    OreRarity     rarity       = OreRarity::COMMON;
    OreTier       oreTier      = OreTier::IRON;
    bool          isKeyAsteroid = false;
    bool          isBoss         = false;
    bool          isMeteor       = false;
    /// 0–3: normale sprite-variant; 4 = key; 5 = boss (`assets/asteroids/*.png`).
    int           spriteVariant  = 0;
    float         bossPhase      = 0.f;
    int           bossPhaseStage    = 1;
    bool          phase2Triggered   = false;
    bool          phase3Triggered   = false;
    float         bossShootTimer    = 0.f;
    float         bossMoveDirTimer  = 0.f;
    sf::Vector2f  bossMoveDir       = { 0.f, 1.f };
    bool          isMiniBoss        = false;
    /// Sleutel-asteroïde: >=0 = vast aantal keys bij vernietiging; -1 = willekeur 1–3.
    int           keyPickupCount = -1;

    int rarityDropMult() const;

    void spawn(AsteroidTier tier,
               sf::Vector2f pos,
               sf::Vector2f vel,
               float        hpMult       = 1.f,
               OreTier      ot           = OreTier::IRON,
               float        radiusScale  = 1.f);

    void spawnKey(sf::Vector2f pos, sf::Vector2f vel, float hpMult,
                   OreTier keyOreTier = OreTier::GOLD,
                   int     forcedKeys = -1,
                   float   radiusScale = 1.f);

    void spawnBoss(float ox, float oy, float areaW, float areaH,
                   float hpMult, OreTier lootTier,
                   float radiusScale = 1.f);

    void spawnMiniBoss(sf::Vector2f pos,
                       float hpMult, OreTier lootTier);

    void spawnBossProjectile(sf::Vector2f origin,
                             sf::Vector2f direction,
                             float hpMult, OreTier tier,
                             float radiusScale = 1.f);

    void spawnMeteor(sf::Vector2f p, sf::Vector2f v,
                      float hpMult, OreTier maxOreTier,
                      float radiusScale = 1.f);

    bool hit(float damage, ParticleSystem& particles);
    void update(float dt, sf::Vector2f playerPos = { 0.f, 0.f });
    void drawShape(sf::RenderTarget& target,
                   float               animTime,
                   const sf::Texture*  keyIconTex = nullptr,
                   const sf::Texture*  bossTex    = nullptr) const;
    /// `tintedSpriteBody`: lichaam is PNG vanuit MiningScreen — sleutel-icoon / HP-balk.
    void drawOverlays(sf::RenderTarget& target,
                      float               animTime,
                      const sf::Font*     labelFont          = nullptr,
                      const sf::Texture*  keyIconTex         = nullptr,
                      const sf::Texture*  bossTex            = nullptr,
                      bool                tintedSpriteBody   = false,
                      float               panelOx            = -1.f,
                      float               panelOy            = -1.f,
                      float               panelW             = -1.f) const;
    void draw(sf::RenderTarget& target,
               float               animTime   = 0.f,
               const sf::Font*     labelFont  = nullptr,
               const sf::Texture*  keyIconTex = nullptr,
               const sf::Texture*  bossTex    = nullptr) const;
    void bounceWalls(float left, float top,
                     float right, float bottom);

private:
    void buildShape();
    void buildKeyOctagon();
    void buildBossShape();
    void buildMiniBossShape();
};

// ─────────────────────────────────────────────────────────────
//  AsteroidManager
// ─────────────────────────────────────────────────────────────
class AsteroidManager {
public:
    AsteroidManager();

    void spawnRandom(float ox, float oy, float areaW, float areaH,
                     float hpMult, OreTier maxTier);

    void maintainField(int targetCount,
                       float ox, float oy, float areaW, float areaH,
                       float hpMult, OreTier maxTier);
    void setKeyAsteroidsEnabled(bool enabled) { m_keyAsteroidsEnabled = enabled; }

    bool trySpawnKey(float ox, float oy, float areaW, float areaH,
                     float hpMult, OreTier keyOreTier, int forcedKeyCount = -1);
    void spawnBonusKeyAsteroid(float ox, float oy, float areaW, float areaH,
                               float hpMult, OreTier keyOreTier, int forcedKeyCount);
    bool trySpawnBoss(float ox, float oy, float areaW, float areaH,
                       float hpMult, OreTier lootTier);
    void spawnMeteorSwarm(float ox, float oy, float areaW, float areaH,
                          int count, float hpMult, OreTier maxOreTier);
    /// Zet max. één meteor uit de shower-wachtrij (één per frame).
    void tickMeteorSpawnQueue();
    void clearMeteorSpawnQueue();
    void setMeteorRadiusScale(float scale) { m_meteorRadiusScale = scale; }
    void setFieldRadiusScale(float scale) { m_fieldRadiusScale = scale; }
    void update(float dt, float ox, float oy, float areaW, float areaH,
                sf::Vector2f playerPos, float hpMult = 1.f,
                OreTier maxTier = OreTier::IRON,
                bool phase3SalvoActive = false);
    /// Alleen meteoren integreren + opruimen (bv. mining-tab gepauzeerd op Easy).
    void updateMeteorsOnly(float dt, float ox, float oy, float areaW,
                           float areaH, sf::Vector2f playerPos);
    void draw(sf::RenderTarget& target,
               float               animTime,
               const sf::Font*     labelFont,
               const sf::Texture*  keyIconTex = nullptr) const;

    /// meteorMinYToAcquire: meteoren met pos.y lager worden genegeerd (bv. paneel-top
    /// + marge) zodat torens ze niet afknallen voordat ze in beeld zijn.
    static constexpr float NEAREST_METEOR_Y_NO_FILTER = -1e9f;
    Asteroid* nearest(sf::Vector2f from,
                        float maxDist                 = 99999.f,
                        float meteorMinYToAcquire     = NEAREST_METEOR_Y_NO_FILTER);

    std::array<Asteroid, MAX_ASTEROIDS>& all() { return m_pool; }
    const std::array<Asteroid, MAX_ASTEROIDS>& all() const { return m_pool; }

    int aliveCount() const { return m_alive; }
    void refreshAliveCount();

    /// Actieve zone-boss op het veld (voor boss-muziek).
    bool hasLivingBoss() const;

    Asteroid* getBoss();
    void spawnBossProjectile(sf::Vector2f origin,
                             sf::Vector2f direction,
                             OreTier      tier, float hpMult);
    void spawnMiniBoss(sf::Vector2f pos, float hpMult, OreTier tier);

    void tickBossShooting(float dt, sf::Vector2f playerPos,
                          float hpMult, OreTier maxTier,
                          bool phase3SalvoActive);

private:
    std::array<Asteroid, MAX_ASTEROIDS> m_pool;
    int                                  m_alive = 0;

    sf::Texture m_bossTex;
    bool        m_bossTexOk = false;

    bool m_keyAsteroidsEnabled = false;

    bool          m_meteorQueueActive   = false;
    int           m_meteorQueueTotal    = 0;
    int           m_meteorQueueSpawned  = 0;
    int           m_meteorQueueLeftCount = 0;
    sf::Vector2f  m_meteorQueueApex{};
    sf::Vector2f  m_meteorQueueDirL{};
    sf::Vector2f  m_meteorQueueDirR{};
    float         m_meteorQueueArmLen   = 0.f;
    sf::Vector2f  m_meteorQueueVel{};
    float         m_meteorQueueHpMult   = 1.f;
    OreTier       m_meteorQueueMaxOre   = OreTier::IRON;
    float         m_meteorRectOx = 0.f;
    float         m_meteorRectOy = 0.f;
    float         m_meteorRectW  = 0.f;
    float         m_meteorRectH  = 0.f;
    float         m_meteorRadiusScale = 1.f;
    float         m_fieldRadiusScale  = 1.f;

    bool spawnOneQueuedMeteor();
    Asteroid* claim();
};
