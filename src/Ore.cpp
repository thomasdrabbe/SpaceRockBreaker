#include "Ore.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>

// ═════════════════════════════════════════════════════════════
//  Constructor
// ═════════════════════════════════════════════════════════════
OreManager::OreManager() {
    // pool zero-initialised; alive=false for all slots
}

// ─────────────────────────────────────────────────────────────
//  claim  — next free slot
// ─────────────────────────────────────────────────────────────
Ore* OreManager::claim() {
    for (auto& o : m_pool)
        if (!o.alive) return &o;
    return nullptr;
}

// ═════════════════════════════════════════════════════════════
//  drop  — spawn ore pieces at an asteroid death position
// ═════════════════════════════════════════════════════════════
void OreManager::drop(sf::Vector2f    pos,
                       sf::Color       color,
                       double          baseValue,
                       int             count,
                       float           oreValueMult,
                       float           oreLuckBonus,
                       ParticleSystem& particles) {

    // Luck: chance to double the drop count
    int finalCount = count;
    if (chance(oreLuckBonus))
        finalCount *= 2;

    for (int i = 0; i < finalCount; i++) {
        Ore* o = claim();
        if (!o) return;

        // Random burst velocity
        float angle = randFloat(0.f, 2.f * PI);
        float speed = randFloat(30.f, 100.f);

        o->pos      = pos + sf::Vector2f(
                            randFloat(-8.f, 8.f),
                            randFloat(-8.f, 8.f));
        o->vel      = { std::cos(angle) * speed,
                        std::sin(angle) * speed };
        o->color    = sf::Color(
                        static_cast<uint8_t>(clamp(
                            color.r + randInt(-20, 20), 0, 255)),
                        static_cast<uint8_t>(clamp(
                            color.g + randInt(-20, 20), 0, 255)),
                        static_cast<uint8_t>(clamp(
                            color.b + randInt(-20, 20), 0, 255)));

        // Value scales with oreValueMult + random ±10 %
        o->value    = baseValue * oreValueMult
                      * randFloat(0.9f, 1.1f);

        o->radius   = randFloat(4.f, 9.f);
        o->lifetime = ORE_LIFETIME + randFloat(-2.f, 2.f);
        o->bobTimer = randFloat(0.f, 2.f * PI);   // stagger bob phase
        o->alive       = true;
        o->collecting  = false;

        // Small collect-sparkle emitted immediately
        particles.emitOreCollect(o->pos,
                                  pos + sf::Vector2f(0, -40),
                                  o->color, 1);
    }
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void OreManager::update(float        dt,
                         sf::Vector2f collectorPos,
                         float        collectRadius,
                         double&      oreOut,
                         int          bulkMultiplier,
                         ParticleSystem& particles) {
    m_alive = 0;

    for (auto& o : m_pool) {
        if (!o.alive) continue;

        o.lifetime -= dt;
        if (o.lifetime <= 0.f) {
            o.alive = false;
            continue;
        }

        float dist = distance(o.pos, collectorPos);
        if (dist <= collectRadius)
            o.collecting = true;

        if (o.collecting) {
            sf::Vector2f dir = normalize(collectorPos - o.pos);
            o.pos += dir * COLLECT_PULL_SPEED * dt;

            if (distance(o.pos, collectorPos) < 8.f) {
                // ── Alleen ore tellen, GEEN credits ──────
                oreOut += static_cast<double>(bulkMultiplier);

                particles.emitSpark(o.pos,
                                    normalize(collectorPos - o.pos),
                                    3);
                o.alive = false;
                continue;
            }
        } else {
            o.vel      *= (1.f - 2.5f * dt);
            o.bobTimer += BOB_SPEED * dt;
            float bobY  = std::sin(o.bobTimer) * BOB_AMP;
            o.pos      += o.vel * dt;
            o.pos.y    += bobY * dt;
        }

        m_alive++;
    }
}
// ═════════════════════════════════════════════════════════════
//  draw
// ═════════════════════════════════════════════════════════════
void OreManager::draw(sf::RenderTarget& target) const {
    sf::CircleShape glow;
    sf::CircleShape core;

    for (const auto& o : m_pool) {
        if (!o.alive) continue;

        // Fade out in last 3 seconds
        float alpha = 1.f;
        if (o.lifetime < 3.f)
            alpha = o.lifetime / 3.f;

        uint8_t a = static_cast<uint8_t>(255 * alpha);

        // Outer glow
        float glowR = o.radius + 5.f;
        glow.setRadius(glowR);
        glow.setOrigin(glowR, glowR);
        glow.setPosition(o.pos);
        glow.setFillColor(sf::Color(
            o.color.r, o.color.g, o.color.b,
            static_cast<uint8_t>(60 * alpha)));
        target.draw(glow);

        // Core gem shape  (slightly squashed circle)
        core.setRadius(o.radius);
        core.setOrigin(o.radius, o.radius);
        core.setPosition(o.pos);
        core.setFillColor(sf::Color(
            o.color.r, o.color.g, o.color.b, a));
        core.setOutlineColor(sf::Color(255, 255, 255,
            static_cast<uint8_t>(120 * alpha)));
        core.setOutlineThickness(1.5f);
        target.draw(core);

        // Bright specular dot
        float specR = o.radius * 0.3f;
        sf::CircleShape spec(specR);
        spec.setOrigin(specR, specR);
        spec.setPosition(o.pos.x - o.radius * 0.25f,
                         o.pos.y - o.radius * 0.25f);
        spec.setFillColor(sf::Color(255, 255, 255,
            static_cast<uint8_t>(180 * alpha)));
        target.draw(spec);
    }
}

// ═════════════════════════════════════════════════════════════
//  collectAll  — vacuum everything on screen
// ═════════════════════════════════════════════════════════════
void OreManager::collectAll(double& oreOut,
                              int     bulkMultiplier) {
    for (auto& o : m_pool) {
        if (!o.alive) continue;
        oreOut += static_cast<double>(bulkMultiplier);
        o.alive = false;
    }
    m_alive = 0;
}

// ═════════════════════════════════════════════════════════════
//  clearAll
// ═════════════════════════════════════════════════════════════
void OreManager::clearAll() {
    for (auto& o : m_pool) o.alive = false;
    m_alive = 0;
}
