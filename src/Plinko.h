#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <array>
#include "Constants.h"
#include "GameState.h"
#include "Particle.h"

// ─────────────────────────────────────────────────────────────
//  Peg
// ─────────────────────────────────────────────────────────────
struct Peg {
    sf::Vector2f pos;
    float        hitFlash = 0.f;
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
               float plinkoLuck);

    bool dropBall(double oreValue, float dropX = -1.f);

    void updateAuto(float dt, double& oreStock,
                    float autoInterval);

    void update(float dt, double& creditsOut,
                float creditMult, int bulkMult,
                ParticleSystem& particles);

    void draw(sf::RenderTarget& target,
              sf::Font&         font) const;

    int   ballsAlive()  const;
    float boardLeft()   const { return m_boardX; }
    float boardTop()    const { return m_boardY; }
    float boardWidth()  const { return m_boardW; }
    float boardHeight() const { return m_boardH; }

private:
    float m_boardX = 0.f, m_boardY = 0.f;
    float m_boardW = 0.f, m_boardH = 0.f;
    int   m_rows   = PLINKO_MIN_ROWS;

    std::vector<Peg>        m_pegs;
    std::vector<PlinkoSlot> m_slots;

    std::array<PlinkoBall, MAX_PLINKO_BALLS> m_balls;

    float m_autoTimer = 0.f;

    void buildPegs();
    void buildSlots(float multBonus, float plinkoLuck);
    void resolvePegCollision(PlinkoBall& ball);
    void resolveWallCollision(PlinkoBall& ball);
    int  findSlot(float x) const;

    PlinkoBall* claimBall();

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
