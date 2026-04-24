#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Constants.h"
#include "Particle.h"
#include "GameState.h"

// ─────────────────────────────────────────────────────────────
//  Single ore pickup
// ─────────────────────────────────────────────────────────────
struct Ore {
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Color    color;
    double       value      = 0.0;   // base credits when collected
    float        radius     = 6.f;
    float        lifetime   = 0.f;   // auto-expire after N seconds
    float        bobTimer   = 0.f;   // sine bob offset
    bool         alive      = false;
    bool         collecting = false; // being pulled toward collector
    sf::Vector2f collectTarget;      // position to fly toward
};

// ─────────────────────────────────────────────────────────────
//  OreManager  — pool + collection logic
// ─────────────────────────────────────────────────────────────
class OreManager {
public:
    OreManager();

    /// Drop ore at pos with given colour/value (from asteroid death)
    void drop(sf::Vector2f    pos,
              sf::Color       color,
              double          baseValue,
              int             count,
              float           oreValueMult,
              float           oreLuckBonus,
              ParticleSystem& particles);

    /// Per-frame: physics, auto-collect, expire
    /// collectorPos    : centre of the collection circle (e.g. ship/base)
    /// collectRadius   : how close ore must be to get pulled in
    /// creditsOut      : credits earned this frame (added to this ref)
    /// bulkMultiplier  : multiply value by this on collection
    void update(float        dt,
                sf::Vector2f collectorPos,
                float        collectRadius,
                double&      oreOut,
                int          bulkMultiplier,
                ParticleSystem& particles);

    void draw(sf::RenderTarget& target) const;

    int  aliveCount() const { return m_alive; }

    /// Force-collect all ore on screen (e.g. prestige cleanup)
    void collectAll(double& oreOut, int bulkMultiplier);
    /// Remove all ore without credit reward
    void clearAll();

private:
    std::array<Ore, MAX_ORE> m_pool;
    int                       m_alive = 0;

    Ore* claim();

    static constexpr float COLLECT_PULL_SPEED = 320.f;  // px/s
    static constexpr float ORE_LIFETIME       = 18.f;   // seconds
    static constexpr float BOB_SPEED          = 2.8f;   // Hz
    static constexpr float BOB_AMP            = 3.f;    // px
};
