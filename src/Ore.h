#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Constants.h"
#include "Particle.h"

// ─────────────────────────────────────────────────────────────
//  Single ore pickup
// ─────────────────────────────────────────────────────────────
struct Ore {
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Color    color;
    float        radius      = 6.f;
    float        lifetime    = 0.f;
    float        bobTimer    = 0.f;
    bool         alive       = false;
    bool         collecting  = false;
    sf::Vector2f collectTarget;
};

// ─────────────────────────────────────────────────────────────
//  OreManager
// ─────────────────────────────────────────────────────────────
class OreManager {
public:
    OreManager();

    /// Drop ore at position — ore gaat naar m_state.ore teller
    /// GEEN credits — alleen via Plinko
    void drop(sf::Vector2f    pos,
              sf::Color       color,
              int             count,
              float           oreLuckBonus,
              ParticleSystem& particles);

    /// Per-frame update — geeft aantal opgepakte ore terug
    void update(float           dt,
                sf::Vector2f    collectorPos,
                float           collectRadius,
                double&         oreOut,
                int             bulkMultiplier,
                ParticleSystem& particles);

    void draw(sf::RenderTarget& target) const;

    int  aliveCount() const { return m_alive; }

    void collectAll(double& oreOut, int bulkMultiplier);
    void clearAll();

private:
    std::array<Ore, MAX_ORE> m_pool;
    int                       m_alive = 0;

    Ore* claim();

    static constexpr float COLLECT_PULL_SPEED = 320.f;
    static constexpr float ORE_LIFETIME       = 18.f;
    static constexpr float BOB_SPEED          = 2.8f;
    static constexpr float BOB_AMP            = 3.f;
};
