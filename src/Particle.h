#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Constants.h"

// ─────────────────────────────────────────────────────────────
//  Particle types
// ─────────────────────────────────────────────────────────────
enum class ParticleType {
    SPARK,
    ORE_PIECE,
    SMOKE,
    CRIT_TEXT,
    ORE_COLLECT,
    EXPLOSION
};

// ─────────────────────────────────────────────────────────────
//  Single particle
// ─────────────────────────────────────────────────────────────
struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Color    color;
    float        lifetime    = 0.f;
    float        maxLifetime = 0.f;
    float        radius      = 0.f;
    float        gravity     = 0.f;
    ParticleType type        = ParticleType::SPARK;
    bool         alive       = false;

    // Voor CRIT_TEXT
    std::string text;
    float       scale = 1.f;
};

// ─────────────────────────────────────────────────────────────
//  ParticleSystem  — fixed-size pool
// ─────────────────────────────────────────────────────────────
class ParticleSystem {
public:
    explicit ParticleSystem(int poolSize = MAX_PARTICLES);

    // ── Emitters ──────────────────────────────────────────
    void emitSpark     (sf::Vector2f pos, sf::Vector2f dir,
                        int count = 6);
    void emitOrePieces (sf::Vector2f pos, sf::Color color,
                        int count = 5);
    void emitSmoke     (sf::Vector2f pos, int count = 4);
    void emitCritText  (sf::Vector2f pos, float damage);
    void emitOreCollect(sf::Vector2f pos, sf::Vector2f target,
                        sf::Color color, int count = 3);
    void emitExplosion (sf::Vector2f pos, float radius,
                        sf::Color color, int count = 20);

    // ── Per-frame ─────────────────────────────────────────
    void update(float dt);
    void draw  (sf::RenderTarget& target, sf::Font& font) const;

    int alive() const { return m_alive; }

private:
    std::vector<Particle> m_pool;
    int                   m_alive = 0;

    Particle* claim();
};
