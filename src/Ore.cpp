#include "Ore.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>

// ═════════════════════════════════════════════════════════════
//  Constructor
// ═════════════════════════════════════════════════════════════
OreManager::OreManager() {}

// ─────────────────────────────────────────────────────────────
//  claim
// ─────────────────────────────────────────────────────────────
Ore* OreManager::claim() {
    for (auto& o : m_pool)
        if (!o.alive) return &o;
    return nullptr;
}

// ═════════════════════════════════════════════════════════════
//  drop
// ═════════════════════════════════════════════════════════════
void OreManager::drop(sf::Vector2f    pos,
                       sf::Color       color,
                       int             count,
                       float           oreLuckBonus,
                       ParticleSystem& particles) {
    // Luck: kans om dubbel te droppen
    int finalCount = count;
    if (chance(oreLuckBonus))
        finalCount *= 2;

    for (int i = 0; i < finalCount; i++) {
        Ore* o = claim();
        if (!o) return;

        float angle = randFloat(0.f, 2.f * PI);
        float speed = randFloat(30.f, 100.f);

        o->pos = pos + sf::Vector2f(randFloat(-8.f, 8.f),
                                     randFloat(-8.f, 8.f));
        o->vel = { std::cos(angle) * speed,
                   std::sin(angle) * speed };

        o->color = sf::Color(
            static_cast<uint8_t>(clamp(
                color.r + randInt(-20, 20), 0, 255)),
            static_cast<uint8_t>(clamp(
                color.g + randInt(-20, 20), 0, 255)),
            static_cast<uint8_t>(clamp(
                color.b + randInt(-20, 20), 0, 255)));

        o->radius     = randFloat(4.f, 9.f);
        o->lifetime   = ORE_LIFETIME + randFloat(-2.f, 2.f);
        o->bobTimer   = randFloat(0.f, 2.f * PI);
        o->alive      = true;
        o->collecting = false;

        particles.emitOreCollect(o->pos,
                                  pos + sf::Vector2f(0, -40),
                                  o->color, 1);
    }
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void OreManager::update(float           dt,
                          sf::Vector2f    collectorPos,
                          float           collectRadius,
                          double&         oreOut,
                          int             bulkMultiplier,
                          ParticleSystem& particles) {
    m_alive = 0;

    for (auto& o : m_pool) {
        if (!o.alive) continue;

        // ── Lifetime ──────────────────────────────────────
        o.lifetime -= dt;
        if (o.lifetime <= 0.f) {
            o.alive = false;
            continue;
        }

        // ── Check collect radius ──────────────────────────
        float dist = distance(o.pos, collectorPos);
        if (dist <= collectRadius)
            o.collecting = true;

        // ── Movement ──────────────────────────────────────
        if (o.collecting) {
            sf::Vector2f dir = normalize(collectorPos - o.pos);
            o.pos += dir * COLLECT_PULL_SPEED * dt;

            if (distance(o.pos, collectorPos) < 8.f) {
                // ── Alleen ore tellen, GEEN credits ───────
                oreOut += static_cast<double>(bulkMultiplier);

                particles.emitSpark(
                    o.pos,
                    normalize(collectorPos - o.pos), 3);
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
    sf::CircleShape spec;

    for (const auto& o : m_pool) {
        if (!o.alive) continue;

        // Fade in laatste 3 seconden
        float   alpha = (o.lifetime < 3.f)
                        ? o.lifetime / 3.f : 1.f;
        uint8_t a     = static_cast<uint8_t>(255 * alpha);

        // Outer glow
        float glowR = o.radius + 5.f;
        glow.setRadius(glowR);
        glow.setOrigin({ glowR, glowR });
        glow.setPosition(o.pos);
        glow.setFillColor(sf::Color(
            o.color.r, o.color.g, o.color.b,
            static_cast<uint8_t>(60 * alpha)));
        target.draw(glow);

        // Core
        core.setRadius(o.radius);
        core.setOrigin({ o.radius, o.radius });
        core.setPosition(o.pos);
        core.setFillColor(sf::Color(
            o.color.r, o.color.g, o.color.b, a));
        core.setOutlineColor(sf::Color(255, 255, 255,
            static_cast<uint8_t>(120 * alpha)));
        core.setOutlineThickness(1.5f);
        target.draw(core);

        // Specular dot
        float specR = o.radius * 0.3f;
        spec.setRadius(specR);
        spec.setOrigin({ specR, specR });
        spec.setPosition({
            o.pos.x - o.radius * 0.25f,
            o.pos.y - o.radius * 0.25f });
        spec.setFillColor(sf::Color(255, 255, 255,
            static_cast<uint8_t>(180 * alpha)));
        spec.setOutlineThickness(0.f);
        target.draw(spec);
    }
}

// ═════════════════════════════════════════════════════════════
//  collectAll
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
