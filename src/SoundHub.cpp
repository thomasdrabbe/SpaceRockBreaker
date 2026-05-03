#include "SoundHub.h"
#include <SFML/Audio/Listener.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

SoundHub gSfx;

namespace {

/// Aantal schoten in `gun sound.mp3` (vaste volgorde, round-robin na 14).
constexpr int kGunShotCount = 14;

void fadeTailInterleaved(std::vector<std::int16_t>& d,
                         unsigned                   ch,
                         unsigned                   fadeFrames) {
    const size_t nf = d.size() / static_cast<size_t>(ch);
    if (nf < 4 || ch < 1u || fadeFrames == 0)
        return;
    fadeFrames = std::min(fadeFrames, static_cast<unsigned>(nf) - 1u);
    const std::uint64_t start = static_cast<std::uint64_t>(nf - fadeFrames);
    for (std::uint64_t fi = start; fi < nf; ++fi) {
        const float t =
            static_cast<float>(fi - start + 1u) / static_cast<float>(fadeFrames);
        const float g = 1.f - t;
        for (unsigned c = 0; c < ch; ++c)
            d[static_cast<size_t>(fi) * ch + c] =
                static_cast<std::int16_t>(
                    static_cast<float>(
                        d[static_cast<size_t>(fi) * ch + c]) * g);
    }
}

/// Pieken zoals in je waveform: start boven hoge drempel, eind na korte “vallei”
/// onder lagere drempel (scheidt 7+7 pieken). Alleen bij precies 14 regio’s OK.
bool splitGunSoundFourteenPeaks(const sf::SoundBuffer&            src,
                                std::vector<sf::SoundBuffer>& out) {
    out.clear();
    const std::int16_t* smp = src.getSamples();
    const std::uint64_t n   = src.getSampleCount();
    const unsigned      ch  = src.getChannelCount();
    const unsigned      rate = src.getSampleRate();
    const std::vector<sf::SoundChannel> chMap = src.getChannelMap();
    if (!smp || ch < 1u || n < static_cast<std::uint64_t>(ch) * 256u)
        return false;

    const std::uint64_t frames = n / static_cast<std::uint64_t>(ch);
    std::vector<float>  env(static_cast<size_t>(frames), 0.f);
    float                 mx = 0.f;
    for (std::uint64_t f = 0; f < frames; ++f) {
        int peak = 0;
        for (unsigned c = 0; c < ch; ++c) {
            int v = static_cast<int>(smp[f * static_cast<std::uint64_t>(ch) + c]);
            if (v < 0)
                v = -v;
            if (v > peak)
                peak = v;
        }
        env[static_cast<size_t>(f)] = static_cast<float>(peak);
        mx                          = std::max(mx, env[static_cast<size_t>(f)]);
    }
    if (mx < 1.f)
        return false;

    const unsigned minLen =
        std::max(20u, static_cast<unsigned>(static_cast<float>(rate) * 0.0018f));
    const unsigned fadeF =
        std::min(120u, std::max(20u, rate / 500u));

    const float startMuls[] = { 0.038f, 0.045f, 0.052f, 0.060f, 0.068f };
    const float endMuls[]   = { 0.010f, 0.013f, 0.016f, 0.019f, 0.023f, 0.028f };
    const unsigned holds[]  = { 28u,  36u,  44u,  52u,  64u,  80u,
                                96u, 112u, 128u, 152u, 176u, 208u, 240u };

    auto detect = [&](float thrStart, float thrEnd, unsigned holdBelow)
        -> std::vector<std::pair<std::uint64_t, std::uint64_t>> {
        std::vector<std::pair<std::uint64_t, std::uint64_t>> r;
        bool     active = false;
        uint64_t a      = 0;
        uint64_t lastHi = 0;
        unsigned below  = 0;
        for (std::uint64_t f = 0; f < frames; ++f) {
            const float e = env[static_cast<size_t>(f)];
            if (!active) {
                if (e >= thrStart) {
                    active = true;
                    a      = f;
                    lastHi = f;
                    below  = 0;
                }
            } else {
                if (e >= thrEnd) {
                    lastHi = f;
                    below  = 0;
                } else {
                    ++below;
                    if (below >= holdBelow) {
                        const std::uint64_t b = lastHi + 1u;
                        if (b > a && b - a >= static_cast<std::uint64_t>(minLen))
                            r.push_back({ a, b });
                        active = false;
                        below  = 0;
                    }
                }
            }
        }
        if (active) {
            const std::uint64_t b = lastHi + 1u;
            if (b > a && b - a >= static_cast<std::uint64_t>(minLen))
                r.push_back({ a, b });
        }
        return r;
    };

    std::vector<std::pair<std::uint64_t, std::uint64_t>> best;
    bool                                                  found = false;
    for (float sm : startMuls) {
        for (float em : endMuls) {
            const float thrEnd  = std::max(90.f, mx * em);
            const float thrStart =
                std::max(thrEnd * 2.1f, std::max(380.f, mx * sm));
            for (unsigned hold : holds) {
                auto r = detect(thrStart, thrEnd, hold);
                if (r.size() == static_cast<size_t>(kGunShotCount)) {
                    best  = std::move(r);
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
        if (found)
            break;
    }
    if (!found)
        return false;

    for (const auto& reg : best) {
        const std::uint64_t a = reg.first;
        const std::uint64_t b = reg.second;
        if (b <= a)
            continue;
        const std::uint64_t sampleCount =
            (b - a) * static_cast<std::uint64_t>(ch);
        std::vector<std::int16_t> chunk(static_cast<size_t>(sampleCount));
        std::memcpy(chunk.data(),
                    smp + a * static_cast<std::uint64_t>(ch),
                    static_cast<size_t>(sampleCount) * sizeof(std::int16_t));
        fadeTailInterleaved(chunk, ch, fadeF);
        sf::SoundBuffer piece;
        if (!piece.loadFromSamples(
                chunk.data(), sampleCount, ch, rate, chMap))
            return false;
        out.push_back(std::move(piece));
    }
    return static_cast<int>(out.size()) == kGunShotCount;
}

bool splitBufferIntoEqualSegments(const sf::SoundBuffer&            src,
                                  std::vector<sf::SoundBuffer>& out,
                                  int                           segments) {
    out.clear();
    if (segments < 1)
        return false;
    const std::int16_t* smp = src.getSamples();
    const std::uint64_t n   = src.getSampleCount();
    const unsigned      ch  = src.getChannelCount();
    const unsigned      rate = src.getSampleRate();
    const std::vector<sf::SoundChannel> chMap = src.getChannelMap();
    if (!smp || ch < 1u)
        return false;
    const std::uint64_t frames = n / static_cast<std::uint64_t>(ch);
    if (frames < static_cast<std::uint64_t>(segments))
        return false;
    const std::uint64_t framesPer =
        frames / static_cast<std::uint64_t>(segments);
    for (int seg = 0; seg < segments; ++seg) {
        const std::uint64_t f0 =
            static_cast<std::uint64_t>(seg) * framesPer;
        const std::uint64_t f1 = (seg == segments - 1)
            ? frames
            : f0 + framesPer;
        const std::uint64_t sampleCount =
            (f1 - f0) * static_cast<std::uint64_t>(ch);
        if (sampleCount == 0u)
            return false;
        sf::SoundBuffer piece;
        if (!piece.loadFromSamples(
                smp + f0 * static_cast<std::uint64_t>(ch),
                sampleCount, ch, rate, chMap))
            return false;
        out.push_back(std::move(piece));
    }
    return static_cast<int>(out.size()) == segments;
}

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

void SoundHub::loadShotVariants() {
    m_shotVariants.clear();
    m_shotRing = 0;

    sf::SoundBuffer gun;
    if (gun.loadFromFile("assets/sounds/gun sound.mp3")
        || gun.loadFromFile("../assets/sounds/gun sound.mp3")) {
        if (splitGunSoundFourteenPeaks(gun, m_shotVariants)
            || splitBufferIntoEqualSegments(gun, m_shotVariants, kGunShotCount))
            return;
        m_shotVariants.clear();
    }

    for (int i = 1; i <= kGunShotCount; ++i) {
        char path[96];
        std::snprintf(path, sizeof path, "assets/sounds/shot_%02d.wav", i);
        sf::SoundBuffer b;
        if (!b.loadFromFile(path)) {
            const std::string alt = std::string("../") + path;
            if (!b.loadFromFile(alt))
                continue;
        }
        m_shotVariants.push_back(std::move(b));
    }
    if (static_cast<int>(m_shotVariants.size()) == kGunShotCount)
        return;

    m_shotVariants.clear();
}

void SoundHub::init() {
    m_ready = false;
    m_warpSound.reset();
    for (auto& o : m_pool)
        o.reset();
    m_next = 0;

    for (int i = 0; i < static_cast<int>(Sfx::COUNT); ++i) {
        m_ok[i] = false;
        if (!tryLoad(i, kRelPaths[i]))
            (void)tryLoad(i, std::string("../") + kRelPaths[i]);
    }

    // Warp: opnieuw proberen (tryLoad kan mislukt zijn); geen strikte m4a-duurcheck.
    {
        const int wi = static_cast<int>(Sfx::Warp);
        auto loadPath = [&](const char* rel) -> bool {
            return m_buf[wi].loadFromFile(rel)
                || m_buf[wi].loadFromFile(std::string("../") + rel);
        };
        if (!m_ok[wi])
            m_ok[wi] = loadPath("assets/sounds/warp.m4a");
        if (!m_ok[wi])
            m_ok[wi] = loadPath("assets/sounds/warp.wav");
        if (!m_ok[wi])
            m_ok[wi] = loadPath("assets/sounds/warp.ogg");
        if (!m_ok[wi])
            m_ok[wi] = loadPath(
                "assets/sounds/freesound_community-warp-speed-6255.mp3");
        const int ex = static_cast<int>(Sfx::Explosion);
        if (!m_ok[wi] && m_ok[ex]) {
            m_buf[wi] = m_buf[ex];
            m_ok[wi]  = true;
        }
        if (m_ok[wi]) {
            m_warpSound.emplace(m_buf[wi]);
            m_warpSound->setRelativeToListener(true);
        }
    }

    sf::Listener::setGlobalVolume(100.f);
    sf::Listener::setPosition({ 0.f, 0.f, 0.f });

    loadShotVariants();
    const int shotIdx = static_cast<int>(Sfx::Shot);
    m_ok[shotIdx]     = m_ok[shotIdx] || !m_shotVariants.empty();

    int fb = -1;
    for (int i = 0; i < static_cast<int>(Sfx::COUNT); ++i) {
        if (m_ok[i]) {
            fb = i;
            break;
        }
    }
    if (fb < 0)
        return;

    for (int i = 0; i < kPool; ++i) {
        m_pool[i].emplace(m_buf[fb]);
        if (m_pool[i])
            m_pool[i]->setRelativeToListener(true);
    }

    m_ready = true;
}

void SoundHub::play(Sfx id) {
    if (!m_ready || m_muted)
        return;
    const int i = static_cast<int>(id);
    if (i < 0 || i >= static_cast<int>(Sfx::COUNT) || !m_ok[i])
        return;

    if (id == Sfx::Warp && m_warpSound) {
        m_warpSound->stop();
        m_warpSound->setRelativeToListener(true);
        m_warpSound->setBuffer(m_buf[i]);
        const float vol = std::min(100.f, m_masterVol * 1.4f);
        m_warpSound->setVolume(vol);
        m_warpSound->setPitch(1.f);
        m_warpSound->play();
        return;
    }

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

    const sf::SoundBuffer* buf = &m_buf[i];
    if (id == Sfx::Shot && !m_shotVariants.empty()) {
        const size_t idx =
            static_cast<size_t>(m_shotRing) % m_shotVariants.size();
        ++m_shotRing;
        buf = &m_shotVariants[idx];
    }

    ch->stop();
    ch->setRelativeToListener(true);
    ch->setBuffer(*buf);
    float vol = m_masterVol;
    if (id == Sfx::Warp)
        vol = std::min(100.f, m_masterVol * 1.4f);
    ch->setVolume(vol);
    float pitch = 1.f;
    if (id == Sfx::UiClick)
        pitch = 0.94f + 0.08f * static_cast<float>(std::fmod(t * 17.0, 1.0));
    else if (id == Sfx::Shot && m_shotVariants.empty())
        pitch = 0.94f + 0.08f * static_cast<float>(std::fmod(t * 17.0, 1.0));
    ch->setPitch(pitch);
    ch->play();
}
