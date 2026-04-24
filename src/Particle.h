#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Constants.h"

// ─────────────────────────────────────────────────────────────
//  Particle types
// ─────────────────────────────────────────────────────────────
enum class ParticleType {
    SPARK,        // bullet impact flash
    ORE_PIECE,    // small chunk when asteroid breaks
    SMOKE,        // slow fading smoke puff
    CRIT_TEXT,    // floating "CRIT!" text particle
    ORE_COLLECT,  // small dot flying toward player
    EXPLOSION     // large asteroid death burst
};

// ─────────────────────────────────────────────────────────────
//  Single particle
// ─────────────────────────────────────────────────────────────
struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Color    color;
    float        lifetime;     // seconds remaining
    float        maxLifetime;
    float        radius;
    float        gravity;      // px/s² downward pull (0 = none)
    ParticleType type;
    bool         alive = false;

    // For CRIT_TEXT
    std::string  text;
    float        scale = 1.f;
};

// ─────────────────────────────────────────────────────────────
//  Particle system  — fixed-size pool, no heap alloc per frame
// ─────────────────────────────────────────────────────────────
class ParticleSystem {
public:
    explicit ParticleSystem(int poolSize = MAX_PARTICLES);

    // ── Emitters ─────────────────────────────────────────
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
	void draw(sf::RenderTarget& target, sf::Font& font) const;   // font used for CRIT_TEXT

    int  alive() const { return m_alive; }

private:
    std::vector<Particle> m_pool;
    int                   m_alive = 0;

    Particle* claim();   // returns next free slot or nullptr if full
};
