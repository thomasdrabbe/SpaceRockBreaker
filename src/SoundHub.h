#pragma once
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Clock.hpp>
#include <array>
#include <optional>
#include <string>

enum class Sfx : int {
    Shot = 0,
    Explosion,
    OreCollect,
    UiClick,
    Warp,
    GameOver,
    BossExplode,
    PlinkoDrop,
    PlinkoScore,
    COUNT
};

class SoundHub {
public:
    void init();
    void setMuted(bool m) { m_muted = m; }
    bool isMuted() const { return m_muted; }
    void play(Sfx id);

private:
    static constexpr int kPool = 12;

    std::array<sf::SoundBuffer, static_cast<int>(Sfx::COUNT)> m_buf{};
    std::array<bool, static_cast<int>(Sfx::COUNT)>          m_ok{};
    std::array<std::optional<sf::Sound>, kPool>            m_pool{};
    int                                                      m_next = 0;
    bool                                                     m_ready = false;
    bool                                                     m_muted = false;
    float                                                    m_masterVol = 55.f;

    sf::Clock m_clk;
    float     m_lastShot   = -1.f;
    float     m_lastOre    = -1.f;
    float     m_lastExpl   = -1.f;
    float     m_lastPlinko = -1.f;

    bool tryLoad(int idx, const std::string& path);
};

extern SoundHub gSfx;
