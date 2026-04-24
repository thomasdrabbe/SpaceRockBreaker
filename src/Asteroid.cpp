#include "Asteroid.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>

// ═════════════════════════════════════════════════════════════
//  Tier tables
// ═════════════════════════════════════════════════════════════
namespace {

struct TierData {
    float     radiusMin, radiusMax;
    float     speedMin,  speedMax;
    float     baseHp;
    sf::Color color;
    OreDrop   ore;
};

// Tier order matches AsteroidTier enum: SMALL, MEDIUM, LARGE, GIANT
constexpr int TIER_COUNT = 4;

const TierData TIER_TABLE[TIER_COUNT] = {
    // SMALL
    { 14.f, 22.f,  60.f, 120.f,  30.f,
      sf::Color(140, 110,  80),
      { sf::Color(180, 140,  60),  2.0, 2 } },

    // MEDIUM
    { 26.f, 38.f,  35.f,  80.f,  90.f,
      sf::Color(100,  80, 130),
      { sf::Color(130,  80, 200),  6.0, 4 } },

    // LARGE
    { 42.f, 58.f,  20.f,  50.f, 240.f,
      sf::Color( 80, 120, 100),
      { sf::Color( 60, 200, 120), 18.0, 7 } },

    // GIANT
    { 64.f, 90.f,  10.f,  28.f, 600.f,
      sf::Color(160,  60,  60),
      { sf::Color(220,  80,  60), 50.0, 14 } },
};

// Spawn weight per tier  (cumulative)
const float TIER_WEIGHTS[TIER_COUNT] = { 0.50f, 0.30f, 0.15f, 0.05f };

AsteroidTier pickTier() {
    float r = randFloat(0.f, 1.f);
    float cum = 0.f;
    for (int i = 0; i < TIER_COUNT; i++) {
        cum += TIER_WEIGHTS[i];
        if (r < cum) return static_cast<AsteroidTier>(i);
    }
    return AsteroidTier::SMALL;
}

} // anonymous namespace

// ═════════════════════════════════════════════════════════════
//  Asteroid::spawn
// ═════════════════════════════════════════════════════════════
void Asteroid::spawn(AsteroidTier t,
                     sf::Vector2f p,
                     sf::Vector2f v,
                     float hpMult) {
    const auto& td = TIER_TABLE[static_cast<int>(t)];

    tier         = t;
    pos          = p;
    vel          = v;
    radius       = randFloat(td.radiusMin, td.radiusMax);
    maxHp        = td.baseHp * hpMult;
    hp           = maxHp;
    color        = td.color;
    oreDrop      = td.ore;
    rotation     = randFloat(0.f, 360.f);
    rotationRate = randFloat(-40.f, 40.f);
    alive        = true;

    buildShape();
}

// ─────────────────────────────────────────────────────────────
//  Build an irregular convex polygon
// ─────────────────────────────────────────────────────────────
void Asteroid::buildShape() {
    const int VERTS = randInt(7, 12);
    shape.setPointCount(static_cast<std::size_t>(VERTS));

    for (int i = 0; i < VERTS; i++) {
        float angle = (i / static_cast<float>(VERTS)) * 2.f * PI;
        float r     = radius * randFloat(0.70f, 1.00f);   // jagged edge
        shape.setPoint(static_cast<std::size_t>(i),
                       { std::cos(angle) * r,
                         std::sin(angle) * r });
    }

    shape.setFillColor(color);
    shape.setOutlineColor(sf::Color(
        static_cast<uint8_t>(std::min(255, color.r + 60)),
        static_cast<uint8_t>(std::min(255, color.g + 60)),
        static_cast<uint8_t>(std::min(255, color.b + 60))));
    shape.setOutlineThickness(1.5f);
    shape.setOrigin(0.f, 0.f);
}

// ═════════════════════════════════════════════════════════════
//  Asteroid::hit
// ═════════════════════════════════════════════════════════════
bool Asteroid::hit(float damage, ParticleSystem& particles) {
    hp -= damage;

    // Small spark on every hit
    particles.emitSpark(pos, normalize(-vel), 4);

    if (hp <= 0.f) {
        alive = false;
        particles.emitExplosion(pos, radius, color, 22);
        particles.emitOrePieces(pos, oreDrop.color, oreDrop.count + 2);
        return true;
    }

    // Smoke when below 50% HP
    if (hp < maxHp * 0.5f)
        particles.emitSmoke(pos, 2);

    return false;
}

// ═════════════════════════════════════════════════════════════
//  Asteroid::update
// ═════════════════════════════════════════════════════════════
void Asteroid::update(float dt) {
    if (!alive) return;
    pos      += vel * dt;
    rotation += rotationRate * dt;
}

// ═════════════════════════════════════════════════════════════
//  Asteroid::draw
// ═════════════════════════════════════════════════════════════
void Asteroid::draw(sf::RenderTarget& target) const {
    if (!alive) return;

    // Draw polygon
    sf::Transform tf;
    tf.translate(pos).rotate(rotation);
    target.draw(shape, tf);

    // HP bar (only if damaged)
    if (hp < maxHp) {
        float barW   = radius * 2.f;
        float filled = barW * (hp / maxHp);
        float barY   = pos.y - radius - 8.f;
        float barX   = pos.x - radius;

        sf::RectangleShape bg({ barW, 4.f });
        bg.setPosition(barX, barY);
        bg.setFillColor(sf::Color(60, 10, 10, 200));
        target.draw(bg);

        sf::RectangleShape fg({ filled, 4.f });
        fg.setPosition(barX, barY);

        // Colour shifts red → yellow → green with HP
        float ratio = hp / maxHp;
        uint8_t r = static_cast<uint8_t>((1.f - ratio) * 255);
        uint8_t g = static_cast<uint8_t>(ratio * 220);
        fg.setFillColor(sf::Color(r, g, 30, 220));
        target.draw(fg);
    }
}

// ═════════════════════════════════════════════════════════════
//  Asteroid::bounceWalls
// ═════════════════════════════════════════════════════════════
void Asteroid::bounceWalls(float left, float top,
                            float right, float bottom) {
    if (pos.x - radius < left)  { pos.x = left  + radius; vel.x = std::abs(vel.x);  }
    if (pos.x + radius > right) { pos.x = right - radius; vel.x = -std::abs(vel.x); }
    if (pos.y - radius < top)   { pos.y = top   + radius; vel.y = std::abs(vel.y);  }
    if (pos.y + radius > bottom){ pos.y = bottom- radius; vel.y = -std::abs(vel.y); }
}

// ═════════════════════════════════════════════════════════════
//  AsteroidManager
// ═════════════════════════════════════════════════════════════
AsteroidManager::AsteroidManager() {
    // pool starts empty — all alive=false by default
}

Asteroid* AsteroidManager::claim() {
    for (auto& a : m_pool)
        if (!a.alive) return &a;
    return nullptr;
}

// ─────────────────────────────────────────────────────────────
//  spawnRandom  — pick a random edge of the mining area
// ─────────────────────────────────────────────────────────────
void AsteroidManager::spawnRandom(float areaW, float areaH,
                                   float hpMult) {
    Asteroid* a = claim();
    if (!a) return;

    AsteroidTier tier = pickTier();
    float r = TIER_TABLE[static_cast<int>(tier)].radiusMax;

    // Pick a random border side: 0=top 1=bottom 2=left 3=right
    int side = randInt(0, 3);
    sf::Vector2f spawnPos, spawnVel;

    float cx = areaW * 0.5f;
    float cy = areaH * 0.5f;
    float speed = randFloat(
        TIER_TABLE[static_cast<int>(tier)].speedMin,
        TIER_TABLE[static_cast<int>(tier)].speedMax);

    switch (side) {
        case 0: spawnPos = { randFloat(r, areaW - r), -r };
                break;
        case 1: spawnPos = { randFloat(r, areaW - r), areaH + r };
                break;
        case 2: spawnPos = { -r, randFloat(r, areaH - r) };
                break;
        default:spawnPos = { areaW + r, randFloat(r, areaH - r) };
                break;
    }

    // Aim loosely toward centre with random spread
    sf::Vector2f dir = normalize({
        cx - spawnPos.x + randFloat(-areaW * 0.3f, areaW * 0.3f),
        cy - spawnPos.y + randFloat(-areaH * 0.3f, areaH * 0.3f)
    });
    spawnVel = { dir.x * speed, dir.y * speed };

    a->spawn(tier, spawnPos, spawnVel, hpMult);
    m_alive++;
}

// ─────────────────────────────────────────────────────────────
//  maintainField  — top up to targetCount
// ─────────────────────────────────────────────────────────────
void AsteroidManager::maintainField(int targetCount,
                                     float areaW, float areaH,
                                     float hpMult) {
    int current = aliveCount();
    for (int i = current; i < targetCount; i++)
        spawnRandom(areaW, areaH, hpMult);
}

// ─────────────────────────────────────────────────────────────
//  update
// ─────────────────────────────────────────────────────────────
void AsteroidManager::update(float dt, float areaW, float areaH) {
    m_alive = 0;
    for (auto& a : m_pool) {
        if (!a.alive) continue;
        a.update(dt);
        a.bounceWalls(0.f, 0.f, areaW, areaH);
        m_alive++;
    }
}

// ─────────────────────────────────────────────────────────────
//  draw
// ─────────────────────────────────────────────────────────────
void AsteroidManager::draw(sf::RenderTarget& target) const {
    for (const auto& a : m_pool)
        if (a.alive) a.draw(target);
}

// ─────────────────────────────────────────────────────────────
//  nearest
// ─────────────────────────────────────────────────────────────
Asteroid* AsteroidManager::nearest(sf::Vector2f from, float maxDist) {
    Asteroid* best   = nullptr;
    float     bestD2 = maxDist * maxDist;

    for (auto& a : m_pool) {
        if (!a.alive) continue;
        float d2 = distanceSq(from, a.pos);
        if (d2 < bestD2) {
            bestD2 = d2;
            best   = &a;
        }
    }
    return best;
}