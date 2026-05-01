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
    float         bossPhase      = 0.f;

    int rarityDropMult() const;

    void spawn(AsteroidTier tier,
               sf::Vector2f pos,
               sf::Vector2f vel,
               float        hpMult = 1.f,
               OreTier      ot     = OreTier::IRON);

    void spawnKey(sf::Vector2f pos, sf::Vector2f vel, float hpMult);

    void spawnBoss(float ox, float oy, float areaW, float areaH,
                   float hpMult, OreTier lootTier);

    bool hit(float damage, ParticleSystem& particles);
    void update(float dt, sf::Vector2f playerPos = { 0.f, 0.f });
    void draw(sf::RenderTarget& target,
               float               animTime   = 0.f,
               const sf::Font*     labelFont  = nullptr) const;
    void bounceWalls(float left, float top,
                     float right, float bottom);

private:
    void buildShape();
    void buildKeyOctagon();
    void buildBossShape();
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
    bool trySpawnKey(float ox, float oy, float areaW, float areaH,
                     float hpMult);
    bool trySpawnBoss(float ox, float oy, float areaW, float areaH,
                       float hpMult, OreTier lootTier);
    void update(float dt, float ox, float oy, float areaW, float areaH,
                sf::Vector2f playerPos);
    void draw(sf::RenderTarget& target,
               float               animTime,
               const sf::Font*     labelFont) const;

    Asteroid* nearest(sf::Vector2f from,
                      float maxDist = 99999.f);

    std::array<Asteroid, MAX_ASTEROIDS>& all() {
        return m_pool;
    }

    int aliveCount() const { return m_alive; }

private:
    std::array<Asteroid, MAX_ASTEROIDS> m_pool;
    int                                  m_alive = 0;

    Asteroid* claim();
};
