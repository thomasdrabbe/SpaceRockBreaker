#include "Particle.h"
#include "Utils.h"
#include <cmath>
#include <sstream>
#include <iomanip>

// ─────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────
ParticleSystem::ParticleSystem(int poolSize) {
    m_pool.resize(poolSize);
}

// ─────────────────────────────────────────────────────────────
//  claim
// ─────────────────────────────────────────────────────────────
Particle* ParticleSystem::claim() {
    for (auto& p : m_pool) {
        if (!p.alive) {
            p       = Particle{};
            p.alive = true;
            m_alive++;
            return &p;
        }
    }
    return nullptr;
}

// ═════════════════════════════════════════════════════════════
//  Emitters
// ═════════════════════════════════════════════════════════════
void ParticleSystem::emitSpark(sf::Vector2f pos,
                                sf::Vector2f dir, int count) {
    for (int i = 0; i < count; i++) {
        Particle* p = claim();
        if (!p) return;

        float speed  = randFloat(80.f, 220.f);
        float spread = randFloat(-0.6f, 0.6f);
        sf::Vector2f d = normalize({
            dir.x + randFloat(-spread, spread),
            dir.y + randFloat(-spread, spread)
        });

        p->pos         = pos;
        p->vel         = { d.x * speed, d.y * speed };
        p->color       = sf::Color(255,
                            static_cast<uint8_t>(randInt(180, 255)),
                            50, 255);
        p->lifetime    = randFloat(0.15f, 0.35f);
        p->maxLifetime = p->lifetime;
        p->radius      = randFloat(2.f, 5.f);
        p->gravity     = 0.f;
        p->type        = ParticleType::SPARK;
    }
}

void ParticleSystem::emitOrePieces(sf::Vector2f pos,
                                    sf::Color color, int count) {
    for (int i = 0; i < count; i++) {
        Particle* p = claim();
        if (!p) return;

        float angle = randFloat(0.f, 2.f * PI);
        float speed = randFloat(40.f, 130.f);

        p->pos         = pos;
        p->vel         = { std::cos(angle) * speed,
                           std::sin(angle) * speed };
        p->color       = color;
        p->lifetime    = randFloat(0.5f, 1.2f);
        p->maxLifetime = p->lifetime;
        p->radius      = randFloat(3.f, 7.f);
        p->gravity     = 60.f;
        p->type        = ParticleType::ORE_PIECE;
    }
}

void ParticleSystem::emitSmoke(sf::Vector2f pos, int count) {
    for (int i = 0; i < count; i++) {
        Particle* p = claim();
        if (!p) return;

        p->pos = pos + sf::Vector2f(randFloat(-8.f, 8.f),
                                     randFloat(-8.f, 8.f));
        p->vel = { randFloat(-15.f, 15.f),
                   randFloat(-30.f, -60.f) };
        uint8_t g  = static_cast<uint8_t>(randInt(100, 160));
        p->color       = sf::Color(g, g, g, 180);
        p->lifetime    = randFloat(0.6f, 1.4f);
        p->maxLifetime = p->lifetime;
        p->radius      = randFloat(8.f, 18.f);
        p->gravity     = 0.f;
        p->type        = ParticleType::SMOKE;
    }
}

void ParticleSystem::emitCritText(sf::Vector2f pos, float damage) {
    Particle* p = claim();
    if (!p) return;

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(0)
       << "CRIT " << damage << "!";

    p->pos         = pos;
    p->vel         = { randFloat(-20.f, 20.f), -80.f };
    p->color       = sf::Color(255, 80, 80, 255);
    p->lifetime    = 1.0f;
    p->maxLifetime = 1.0f;
    p->radius      = 0.f;
    p->gravity     = 0.f;
    p->type        = ParticleType::CRIT_TEXT;
    p->text        = ss.str();
    p->scale       = 1.f;
}

void ParticleSystem::emitOreCollect(sf::Vector2f pos,
                                     sf::Vector2f target,
                                     sf::Color color, int count) {
    for (int i = 0; i < count; i++) {
        Particle* p = claim();
        if (!p) return;

        sf::Vector2f dir = normalize(target - pos);
        float speed      = randFloat(120.f, 200.f);

        p->pos  = pos + sf::Vector2f(randFloat(-6.f, 6.f),
                                      randFloat(-6.f, 6.f));
        p->vel  = { dir.x * speed, dir.y * speed };
        p->color       = color;
        p->lifetime    = randFloat(0.3f, 0.6f);
        p->maxLifetime = p->lifetime;
        p->radius      = randFloat(3.f, 6.f);
        p->gravity     = 0.f;
        p->type        = ParticleType::ORE_COLLECT;
    }
}

void ParticleSystem::emitExplosion(sf::Vector2f pos,
                                    float        radius,
                                    sf::Color    color,
                                    int          count) {
    for (int i = 0; i < count; i++) {
        Particle* p = claim();
        if (!p) return;

        float   angle = randFloat(0.f, 2.f * PI);
        float   speed = randFloat(50.f, radius * 2.5f);
        uint8_t fade  = static_cast<uint8_t>(randInt(160, 255));

        p->pos  = pos;
        p->vel  = { std::cos(angle) * speed,
                    std::sin(angle) * speed };
        p->color       = sf::Color(color.r, color.g, color.b, fade);
        p->lifetime    = randFloat(0.4f, 0.9f);
        p->maxLifetime = p->lifetime;
        p->radius      = randFloat(4.f, radius * 0.35f);
        p->gravity     = 20.f;
        p->type        = ParticleType::EXPLOSION;
    }
}

// ═════════════════════════════════════════════════════════════
//  Update
// ═════════════════════════════════════════════════════════════
void ParticleSystem::update(float dt) {
    m_alive = 0;
    for (auto& p : m_pool) {
        if (!p.alive) continue;

        p.lifetime -= dt;
        if (p.lifetime <= 0.f) {
            p.alive = false;
            continue;
        }
        m_alive++;

        p.vel.y += p.gravity * dt;
        p.pos   += p.vel * dt;

        if (p.type == ParticleType::SMOKE)
            p.radius += 12.f * dt;

        if (p.type == ParticleType::CRIT_TEXT)
            p.scale = 1.f + 0.5f *
                      (1.f - p.lifetime / p.maxLifetime);
    }
}

// ═════════════════════════════════════════════════════════════
//  Draw
// ═════════════════════════════════════════════════════════════
void ParticleSystem::draw(sf::RenderTarget& target,
                           sf::Font&         font) const {
    sf::CircleShape circle;

    for (const auto& p : m_pool) {
        if (!p.alive) continue;

        float   t     = p.lifetime / p.maxLifetime;
        uint8_t alpha = static_cast<uint8_t>(p.color.a * t);

        if (p.type == ParticleType::CRIT_TEXT) {
            // ── Floating crit text ────────────────────────
            sf::Text label(font);
            label.setString(p.text);
            label.setCharacterSize(
                static_cast<unsigned>(16 * p.scale));
            label.setFillColor(sf::Color(
                p.color.r, p.color.g, p.color.b, alpha));
            label.setStyle(sf::Text::Bold);
            label.setPosition(p.pos);
            target.draw(label);
        } else {
            // ── Circle particle ───────────────────────────
            circle.setRadius(p.radius);
            circle.setOrigin({p.radius, p.radius});
            circle.setPosition(p.pos);
            circle.setFillColor(sf::Color(
                p.color.r, p.color.g, p.color.b, alpha));
            target.draw(circle);
        }
    }
}
