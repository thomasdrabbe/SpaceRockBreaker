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
    OreRarity    rarity    = OreRarity::COMMON;
    OreTier      oreTier = OreTier::IRON;
    // In Asteroid.h, public:
    int rarityDropMult() const;  // returns countMult based on rarity

    void spawn(AsteroidTier tier,
               sf::Vector2f pos,
               sf::Vector2f vel,
               float        hpMult = 1.f,
               OreTier      ot     = OreTier::IRON);

    bool hit(float damage, ParticleSystem& particles);
    void update(float dt);
    void draw(sf::RenderTarget& target) const;
    void bounceWalls(float left, float top,
                     float right, float bottom);

private:
    void buildShape();
};

// ─────────────────────────────────────────────────────────────
//  AsteroidManager
// ─────────────────────────────────────────────────────────────
class AsteroidManager {
public:
    AsteroidManager();

    // In AsteroidManager:
    void spawnRandom(float areaW, float areaH,
                     float hpMult, OreTier maxTier);

    void maintainField(int targetCount,
                       float areaW, float areaH,
                       float hpMult, OreTier maxTier);
    void update(float dt, float areaW, float areaH);
    void draw(sf::RenderTarget& target) const;

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
