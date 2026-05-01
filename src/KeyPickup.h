#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Constants.h"
#include "Particle.h"

struct KeyPickup {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float        lifetime   = 0.f;
    float        bobTimer   = 0.f;
    float        radius     = 6.f;
    bool         alive      = false;
    bool         collecting = false;
};

class KeyPickupManager {
public:
    KeyPickupManager() = default;

    void drop(sf::Vector2f pos, int count, ParticleSystem& particles);

    void update(float            dt,
                sf::Vector2f     collectorPos,
                float            collectRadius,
                int&             keysOut,
                ParticleSystem&  particles);

    void draw(sf::RenderTarget& target) const;

    int aliveCount() const { return m_alive; }

    void collectAll(int& keysOut);
    void clearAll();

private:
    std::array<KeyPickup, MAX_KEY_PICKUPS> m_pool{};
    int                                    m_alive = 0;

    KeyPickup* claim();

    static constexpr float COLLECT_PULL_SPEED = 320.f;
    static constexpr float KEY_LIFETIME       = 22.f;
    static constexpr float BOB_SPEED          = 2.8f;
    static constexpr float BOB_AMP            = 3.f;
};
