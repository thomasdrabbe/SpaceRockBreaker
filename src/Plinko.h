#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <utility>
#include <string>
#include <vector>
#include <array>
#include "Constants.h"
#include "GameState.h"
#include "Particle.h"

// ─────────────────────────────────────────────────────────────
//  Peg-rarity (Plinko) — kleuren gelijk aan bord + basis-legenda
// ─────────────────────────────────────────────────────────────
struct PlinkoPegRarity {
    static sf::Color fillColor(OreRarity r);
    static float     bonusMult(OreRarity r);
};

// ─────────────────────────────────────────────────────────────
//  Peg
// ─────────────────────────────────────────────────────────────
struct Peg {
    sf::Vector2f pos;
    float        hitFlash = 0.f;
    OreRarity    pegRarity = OreRarity::COMMON;
    int          cellRow   = -1;
    int          cellCol   = -1;
};

// ─────────────────────────────────────────────────────────────
//  Slot
// ─────────────────────────────────────────────────────────────
struct PlinkoSlot {
    sf::Vector2f pos;
    float        width;
    float        multiplier;
    float        flashTimer = 0.f;
    sf::Color    color;
};

// ─────────────────────────────────────────────────────────────
//  Ball
// ─────────────────────────────────────────────────────────────
struct PlinkoBall {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float        radius   = PLINKO_BALL_RADIUS;
    bool         alive    = false;
    bool         scored   = false;
    double       oreValue = 0.0;
};

// ─────────────────────────────────────────────────────────────
//  PlinkoBoard
// ─────────────────────────────────────────────────────────────
class PlinkoBoard {
public:
    PlinkoBoard();

    void build(int   rows,
           float boardX, float boardY,
           float boardW, float boardH,
           float multBonus,
           float plinkoLuck,
           float scale,
           float pegHitRadius,
           float pegBounceMult);

    bool dropBall(double oreValue, float dropX = -1.f);

    void updateAuto(float dt, double& oreStock,
                    float autoInterval);

    void update(float dt, double& creditsOut,
                float creditMult, int bulkMult,
                ParticleSystem& particles);

    void draw(sf::RenderTarget& target,
              sf::Font&         font) const;

    /// Wis persistente peg-rarity (na load ander slot / nieuw spel).
    void resetGoldenPegRarityState();

    /// Past Golden-Peg rolls toe: alleen extra rolls bij hogere chest-level;
    /// zelfde (rij,kolom) blijft zeldzame kleur na rebuild.
    void syncGoldenPegChestRarities(int goldenPegChestLevel);

    int   ballsAlive()  const;
    float boardLeft()   const { return m_boardX; }
    float boardTop()    const { return m_boardY; }
    float boardWidth()  const { return m_boardW; }
    float boardHeight() const { return m_boardH; }

private:
    float m_boardX = 0.f, m_boardY = 0.f;
    float m_boardW = 0.f, m_boardH = 0.f;
    int   m_rows   = PLINKO_MIN_ROWS;
    float m_scale = 1.f;

    float m_pegHitRadius  = PLINKO_PEG_RADIUS;
    float m_pegDrawRadius = PLINKO_PEG_RADIUS;
    float m_pegBounceMult = 1.f;

    std::vector<Peg>        m_pegs;
    std::vector<PlinkoSlot> m_slots;

    std::array<PlinkoBall, MAX_PLINKO_BALLS> m_balls;

    float m_autoTimer = 0.f;

    std::map<std::pair<int, int>, OreRarity> m_pegRarityByCell;
    int m_lastBuildRows               = -1;
    int m_syncedGoldenPegChestLevel   = -1;

    void buildPegs();
    void applyPegRarityRolls(int rollCount);
    void buildSlots(float multBonus, float plinkoLuck);
    void resolvePegCollision(PlinkoBall& ball,
                             double&     creditsOut,
                             float       creditMult);
    void resolveWallCollision(PlinkoBall& ball);
    int  findSlot(float x) const;

    PlinkoBall* claimBall();

    struct PegCreditPopup {
        sf::Vector2f pos{};
        std::string  text;
        float        age = 0.f;
    };
    std::vector<PegCreditPopup> m_pegCreditPopups;

    void pushPegCreditPopup(sf::Vector2f pegCenter, double credits);
    void updatePegCreditPopups(float dt);

    static float slotMult(int slotIdx, int totalSlots,
                          float bonus, float luck);

    static constexpr float WALL_BOUNCE   = 0.55f;
    static constexpr float PEG_BOUNCE    = 0.45f;
    static constexpr float BALL_FRICTION = 0.992f;
    static constexpr float DROP_SPEED_MIN= 80.f;
    static constexpr float DROP_SPEED_MAX= 140.f;
    static constexpr float SLOT_HEIGHT   = 40.f;
    static constexpr float AUTO_INTERVAL = 1.2f;
};
