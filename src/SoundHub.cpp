#include "SoundHub.h"
#include <cmath>

SoundHub gSfx;

namespace {

const char* kRelPaths[static_cast<int>(Sfx::COUNT)] = {
    "assets/sounds/shot.wav",
    "assets/sounds/explosion.wav",
    "assets/sounds/ore.wav",
    "assets/sounds/ui.wav",
    "assets/sounds/warp.wav",
    "assets/sounds/gameover.wav",
    "assets/sounds/boss.wav",
    "assets/sounds/plinko_drop.wav",
    "assets/sounds/plinko_score.wav",
};

} // namespace

bool SoundHub::tryLoad(int idx, const std::string& path) {
    if (idx < 0 || idx >= static_cast<int>(Sfx::COUNT))
        return false;
    m_ok[idx] = m_buf[idx].loadFromFile(path);
    return m_ok[idx];
}

void SoundHub::init() {
    m_ready = false;
    for (auto& o : m_pool)
        o.reset();
    m_next = 0;

    for (int i = 0; i < static_cast<int>(Sfx::COUNT); ++i) {
        m_ok[i] = false;
        if (!tryLoad(i, kRelPaths[i]))
            (void)tryLoad(i, std::string("../") + kRelPaths[i]);
    }

    int fb = -1;
    for (int i = 0; i < static_cast<int>(Sfx::COUNT); ++i) {
        if (m_ok[i]) {
            fb = i;
            break;
        }
    }
    if (fb < 0)
        return;

    for (int i = 0; i < kPool; ++i)
        m_pool[i].emplace(m_buf[fb]);

    m_ready = true;
}

void SoundHub::play(Sfx id) {
    if (!m_ready || m_muted)
        return;
    const int i = static_cast<int>(id);
    if (i < 0 || i >= static_cast<int>(Sfx::COUNT) || !m_ok[i])
        return;

    const float t = m_clk.getElapsedTime().asSeconds();

    if (id == Sfx::Shot) {
        if (t - m_lastShot < 0.05f)
            return;
        m_lastShot = t;
    } else if (id == Sfx::OreCollect) {
        if (t - m_lastOre < 0.07f)
            return;
        m_lastOre = t;
    } else if (id == Sfx::Explosion) {
        if (t - m_lastExpl < 0.04f)
            return;
        m_lastExpl = t;
    } else if (id == Sfx::PlinkoScore) {
        if (t - m_lastPlinko < 0.06f)
            return;
        m_lastPlinko = t;
    }

    std::optional<sf::Sound>& ch = m_pool[m_next];
    m_next                       = (m_next + 1) % kPool;
    if (!ch)
        return;

    ch->setBuffer(m_buf[i]);
    ch->setVolume(m_masterVol);
    float pitch = 1.f;
    if (id == Sfx::Shot || id == Sfx::UiClick)
        pitch = 0.94f + 0.08f * static_cast<float>(std::fmod(t * 17.0, 1.0));
    ch->setPitch(pitch);
    ch->play();
}
