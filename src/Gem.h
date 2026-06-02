#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Constants.h"
#include "Particle.h"

struct GemPickup {
    sf::Vector2f pos;
    GemType      type      = GemType::RUBY;
    bool         alive     = false;
    float        glowTimer = 0.f;
};

class GemPickupManager {
public:
    void setTextures(const std::array<const sf::Texture*, GEM_TYPE_COUNT_INT>* tex);

    void drop(sf::Vector2f pos, GemType type, ParticleSystem& particles);

    void update(float            dt,
                sf::Vector2f     collectorPos,
                float            collectRadius,
                int&             collectedOut,
                GemType&         collectedTypeOut,
                ParticleSystem&  particles);

    void collectAll(int& countOut, GemType& typeOut);
    void clearAll();
    void draw(sf::RenderTarget& target) const;
    int  aliveCount() const { return m_alive; }

private:
    std::array<GemPickup, MAX_GEM_PICKUPS> m_pool{};
    int                                    m_alive = 0;
    const std::array<const sf::Texture*, GEM_TYPE_COUNT_INT>* m_tex = nullptr;

    GemPickup* claim();
};
