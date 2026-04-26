#include "Asteroid.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>

// ═════════════════════════════════════════════════════════════
//  Anonieme namespace — alle tabellen en helpers
// ═════════════════════════════════════════════════════════════
namespace {

// ── Asteroid size/speed/hp tabel ─────────────────────────────
struct TierData {
    float     radiusMin, radiusMax;
    float     speedMin,  speedMax;
    float     baseHp;
};

const TierData TIER_TABLE[4] = {
    { 14.f, 22.f, 60.f, 120.f,  30.f },   // SMALL
    { 26.f, 38.f, 35.f,  80.f,  90.f },   // MEDIUM
    { 42.f, 58.f, 20.f,  50.f, 240.f },   // LARGE
    { 64.f, 90.f, 10.f,  28.f, 600.f },   // GIANT
};

const float TIER_WEIGHTS[4] = { 0.50f, 0.30f, 0.15f, 0.05f };

AsteroidTier pickTier() {
    float r   = randFloat(0.f, 1.f);
    float cum = 0.f;
    for (int i = 0; i < 4; i++) {
        cum += TIER_WEIGHTS[i];
        if (r < cum) return static_cast<AsteroidTier>(i);
    }
    return AsteroidTier::SMALL;
}

// ── Ore tier tabel ────────────────────────────────────────────
struct OreTierData {
    sf::Color asteroidColor;
    sf::Color oreColor;
    double    baseValue;
    int       baseCount;   // ore drops per asteroid size (SMALL)
};

const OreTierData ORE_TIER_TABLE[static_cast<int>(OreTier::ORE_TIER_COUNT)] = {
    { sf::Color( 90,  90,  95), sf::Color(140, 140, 150),    1.0, 2  }, // IRON
    { sf::Color(140,  80,  30), sf::Color(200, 120,  50),    3.0, 2  }, // BRONZE
    { sf::Color(160, 165, 175), sf::Color(210, 215, 225),    8.0, 3  }, // SILVER
    { sf::Color(180, 150,  20), sf::Color(255, 215,  50),   20.0, 3  }, // GOLD
    { sf::Color( 60, 180, 200), sf::Color(140, 230, 255),   55.0, 4  }, // DIAMOND
    { sf::Color( 80, 110, 190), sf::Color(160, 185, 255),  140.0, 4  }, // PLATINUM
    { sf::Color( 50,  70, 150), sf::Color( 90, 120, 220),  380.0, 5  }, // TITANIUM
    { sf::Color( 80,  30, 110), sf::Color(160,  60, 220), 1000.0, 5  }, // IRIDIUM
};

// Drop count scales with asteroid size tier
const int ORE_COUNT_BY_SIZE[4] = { 2, 4, 7, 14 };

OreTier pickOreTier(OreTier maxTier) {
    int max = static_cast<int>(maxTier);
    if (max == 0) return OreTier::IRON;

    float weights[8] = {};
    float total = 0.f;
    float w = 1.f;
    for (int i = 0; i <= max; i++) {
        weights[i] = w;
        total += w;
        w *= 0.40f;
    }

    float r = randFloat(0.f, total);
    float cum = 0.f;
    for (int i = 0; i <= max; i++) {
        cum += weights[i];
        if (r < cum) return static_cast<OreTier>(i);
    }
    return OreTier::IRON;
}

// ── Asteroid rarity tabel ────────────────────────────────────
struct AsteroidRarityData {
    sf::Color outlineColor;
    float     outlineThick;
    int       countMult;
    float     weight;
};

const AsteroidRarityData ASTEROID_RARITY[6] = {
    { sf::Color(160, 160, 165), 1.5f, 1,  50.0f },  // Common
    { sf::Color( 80, 200,  80), 2.0f, 1,  25.0f },  // Uncommon
    { sf::Color( 70, 130, 255), 2.5f, 2,  14.0f },  // Rare
    { sf::Color(185,  60, 255), 3.0f, 3,   7.5f },  // Epic
    { sf::Color(220,  50,  50), 3.5f, 5,   2.8f },  // Mythic
    { sf::Color(255, 170,   0), 4.5f, 8,   0.7f },  // Legendary
};

OreRarity rollAsteroidRarity() {
    float total = 0.f;
    for (const auto& r : ASTEROID_RARITY) total += r.weight;
    float roll = randFloat(0.f, total);
    float cum  = 0.f;
    for (int i = 0; i < 6; i++) {
        cum += ASTEROID_RARITY[i].weight;
        if (roll < cum) return static_cast<OreRarity>(i);
    }
    return OreRarity::COMMON;
}

} // anonymous namespace

// ═════════════════════════════════════════════════════════════
//  Asteroid::rarityDropMult
// ═════════════════════════════════════════════════════════════
int Asteroid::rarityDropMult() const {
    return ASTEROID_RARITY[static_cast<int>(rarity)].countMult;
}

// ═════════════════════════════════════════════════════════════
//  Asteroid::spawn
// ═════════════════════════════════════════════════════════════
void Asteroid::spawn(AsteroidTier t,
                     sf::Vector2f p,
                     sf::Vector2f v,
                     float        hpMult,
                     OreTier      ot) {
    const auto& td  = TIER_TABLE[static_cast<int>(t)];
    const auto& otd = ORE_TIER_TABLE[static_cast<int>(ot)];

    tier         = t;
    oreTier      = ot;
    rarity       = rollAsteroidRarity();
    pos          = p;
    vel          = v;
    radius       = randFloat(td.radiusMin, td.radiusMax);
    maxHp        = td.baseHp * hpMult;
    hp           = maxHp;
    color        = otd.asteroidColor;
    rotation     = randFloat(0.f, 360.f);
    rotationRate = randFloat(-40.f, 40.f);
    alive        = true;

    oreDrop.color = otd.oreColor;
    oreDrop.value = otd.baseValue;
    oreDrop.count = ORE_COUNT_BY_SIZE[static_cast<int>(t)];

    buildShape();
}

// ─────────────────────────────────────────────────────────────
//  Asteroid::buildShape
// ─────────────────────────────────────────────────────────────
void Asteroid::buildShape() {
    const int VERTS = randInt(7, 12);
    shape.setPointCount(static_cast<std::size_t>(VERTS));

    for (int i = 0; i < VERTS; i++) {
        float angle = (i / static_cast<float>(VERTS)) * 2.f * PI;
        float r     = radius * randFloat(0.70f, 1.00f);
        shape.setPoint(static_cast<std::size_t>(i),
                       { std::cos(angle) * r,
                         std::sin(angle) * r });
    }

    shape.setFillColor(color);

    // Outline kleur en dikte komen van de rarity
    const auto& rd = ASTEROID_RARITY[static_cast<int>(rarity)];
    shape.setOutlineColor(rd.outlineColor);
    shape.setOutlineThickness(rd.outlineThick);
}

// ═════════════════════════════════════════════════════════════
//  Asteroid::hit
// ═════════════════════════════════════════════════════════════
bool Asteroid::hit(float damage, ParticleSystem& particles) {
    hp -= damage;
    particles.emitSpark(pos, normalize(-vel), 4);

    if (hp <= 0.f) {
        alive = false;
        particles.emitExplosion(pos, radius, color, 22);
        particles.emitOrePieces(pos, oreDrop.color,
                                 oreDrop.count + 2);
        return true;
    }

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

    sf::Transform tf;
    tf.translate(pos).rotate(sf::degrees(rotation));
    target.draw(shape, tf);

    // HP bar
    if (hp < maxHp) {
        float barW   = radius * 2.f;
        float filled = barW * (hp / maxHp);
        float barY   = pos.y - radius - 8.f;
        float barX   = pos.x - radius;

        sf::RectangleShape bg(sf::Vector2f{ barW, 4.f });
        bg.setPosition({ barX, barY });
        bg.setFillColor(sf::Color(60, 10, 10, 200));
        target.draw(bg);

        sf::RectangleShape fg(sf::Vector2f{ filled, 4.f });
        fg.setPosition({ barX, barY });
        float   ratio = hp / maxHp;
        uint8_t r     = static_cast<uint8_t>((1.f - ratio) * 255);
        uint8_t g     = static_cast<uint8_t>(ratio * 220);
        fg.setFillColor(sf::Color(r, g, 30, 220));
        target.draw(fg);
    }
}

// ═════════════════════════════════════════════════════════════
//  Asteroid::bounceWalls
// ═════════════════════════════════════════════════════════════
void Asteroid::bounceWalls(float left, float top,
                            float right, float bottom) {
    if (pos.x - radius < left)  { pos.x = left  + radius; vel.x =  std::abs(vel.x); }
    if (pos.x + radius > right) { pos.x = right - radius; vel.x = -std::abs(vel.x); }
    if (pos.y - radius < top)   { pos.y = top   + radius; vel.y =  std::abs(vel.y); }
    if (pos.y + radius > bottom){ pos.y = bottom- radius; vel.y = -std::abs(vel.y); }
}

// ═════════════════════════════════════════════════════════════
//  AsteroidManager
// ═════════════════════════════════════════════════════════════
AsteroidManager::AsteroidManager() {}

Asteroid* AsteroidManager::claim() {
    for (auto& a : m_pool)
        if (!a.alive) return &a;
    return nullptr;
}

void AsteroidManager::spawnRandom(float areaW, float areaH,
                                   float hpMult, OreTier maxTier) {
    Asteroid* a = claim();
    if (!a) return;

    AsteroidTier t  = pickTier();
    OreTier      ot = pickOreTier(maxTier);

    const auto& td = TIER_TABLE[static_cast<int>(t)];
    float r     = td.radiusMax;
    float speed = randFloat(td.speedMin, td.speedMax);

    int side = randInt(0, 3);
    sf::Vector2f spawnPos;
    float cx = areaW * 0.5f;
    float cy = areaH * 0.5f;

    switch (side) {
        case 0: spawnPos = { randFloat(r, areaW - r), -r };          break;
        case 1: spawnPos = { randFloat(r, areaW - r), areaH + r };   break;
        case 2: spawnPos = { -r,          randFloat(r, areaH - r) }; break;
        default:spawnPos = { areaW + r,   randFloat(r, areaH - r) }; break;
    }

    sf::Vector2f dir = normalize({
        cx - spawnPos.x + randFloat(-areaW * 0.3f, areaW * 0.3f),
        cy - spawnPos.y + randFloat(-areaH * 0.3f, areaH * 0.3f)
    });

    a->spawn(t, spawnPos, dir * speed, hpMult, ot);
    m_alive++;
}

void AsteroidManager::maintainField(int targetCount,
                                     float areaW, float areaH,
                                     float hpMult, OreTier maxTier) {
    int current = aliveCount();
    for (int i = current; i < targetCount; i++)
        spawnRandom(areaW, areaH, hpMult, maxTier);
}

void AsteroidManager::update(float dt, float areaW, float areaH) {
    m_alive = 0;
    for (auto& a : m_pool) {
        if (!a.alive) continue;
        a.update(dt);
        a.bounceWalls(0.f, 0.f, areaW, areaH);
        m_alive++;
    }
}

void AsteroidManager::draw(sf::RenderTarget& target) const {
    for (const auto& a : m_pool)
        if (a.alive) a.draw(target);
}

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
