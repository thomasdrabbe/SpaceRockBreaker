#include "SoundHub.h"
#include "Utils.h"
#include <SFML/Audio/Listener.hpp>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

SoundHub gSfx;

namespace {

std::filesystem::path resolvedFilePathIfExists(const std::string& relOrAbs) {
    namespace fs = std::filesystem;
    const fs::path p(resolveAssetPath(relOrAbs));
    std::error_code ec;
    if (fs::is_regular_file(p, ec))
        return p;
    return {};
}

bool openMusicFromAsset(sf::Music& music, const char* relPath) {
    const auto p = resolvedFilePathIfExists(relPath);
    if (p.empty())
        return false;
    return music.openFromFile(p);
}

/// Maximaal aantal losse `shot_XX.wav`-fallbacks om te proberen.
constexpr int kGunShotWavFileMax = 14;
/// Minimaal zoveel varianten om schoten uit `gun sound.mp3` te gebruiken.
constexpr int kGunShotMinVariants  = 10;

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

[[nodiscard]] static int maxAbsSampleSoundBuffer(const sf::SoundBuffer& buf) {
    const std::int16_t* s = buf.getSamples();
    const std::uint64_t n = buf.getSampleCount();
    int                  m = 0;
    for (std::uint64_t i = 0; i < n; ++i) {
        int v = static_cast<int>(s[i]);
        if (v < 0)
            v = -v;
        if (v > m)
            m = v;
    }
    return m;
}

/// Pieken in de waveform: `targetCount` regio’s (bv. 10 = 5 schoten + pauze + 5).
/// Stille “schoten” (pauze als piek) worden afgewezen → volgende split-methode.
bool splitGunSoundByPeaks(const sf::SoundBuffer&            src,
                          std::vector<sf::SoundBuffer>& out,
                          int                           targetCount) {
    out.clear();
    const std::int16_t* smp = src.getSamples();
    const std::uint64_t n   = src.getSampleCount();
    const unsigned      ch  = src.getChannelCount();
    const unsigned      rate = src.getSampleRate();
    const std::vector<sf::SoundChannel> chMap = src.getChannelMap();
    if (!smp || ch < 1u || n < static_cast<std::uint64_t>(ch) * 256u)
        return false;

    int globalAbsMax = 0;
    for (std::uint64_t i = 0; i < n; ++i) {
        int v = static_cast<int>(smp[i]);
        if (v < 0)
            v = -v;
        if (v > globalAbsMax)
            globalAbsMax = v;
    }
    if (globalAbsMax < 8)
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
                if (r.size() == static_cast<size_t>(targetCount)) {
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
    if (static_cast<int>(out.size()) != targetCount) {
        out.clear();
        return false;
    }
    const int quietThr =
        std::max(120, static_cast<int>(static_cast<float>(globalAbsMax) * 0.06f));
    for (const auto& pb : out) {
        if (maxAbsSampleSoundBuffer(pb) < quietThr) {
            out.clear();
            return false;
        }
    }
    return true;
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
    /// Geen `warp.wav`/`warp.m4a` in release; SFML ondersteunt m4a niet.
    "assets/sounds/freesound_community-warp-speed-6255.mp3",
    "assets/sounds/gameover.wav",
    "assets/sounds/boss.wav",
    "assets/sounds/plinko_drop.wav",
    "assets/sounds/plinko_score.wav",
    "assets/sounds/chest_open.wav",
    "assets/sounds/chest_loot.wav",
    "assets/sounds/levelup.mp3",
};

} // namespace

bool SoundHub::tryLoad(int idx, const std::string& relOrAbs) {
    if (idx < 0 || idx >= static_cast<int>(Sfx::COUNT))
        return false;
    const auto p = resolvedFilePathIfExists(relOrAbs);
    if (p.empty()) {
        m_ok[idx] = false;
        return false;
    }
    m_ok[idx] = m_buf[idx].loadFromFile(p);
    return m_ok[idx];
}

void SoundHub::loadShotVariants() {
    m_shotVariants.clear();
    m_shotRing = 0;

    sf::SoundBuffer gun;
    const std::string gunPath = resolveAssetPath("assets/sounds/gun sound.mp3");
    if (gun.loadFromFile(std::filesystem::path(gunPath))) {
        if (splitGunSoundByPeaks(gun, m_shotVariants, 10)
            || splitGunSoundByPeaks(gun, m_shotVariants, 14)
            || splitBufferIntoEqualSegments(gun, m_shotVariants, 10)
            || splitBufferIntoEqualSegments(gun, m_shotVariants, 14))
            return;
        m_shotVariants.clear();
    }

    for (int i = 1; i <= kGunShotWavFileMax; ++i) {
        char rel[96];
        std::snprintf(rel, sizeof rel, "assets/sounds/shot_%02d.wav", i);
        const auto p = resolvedFilePathIfExists(rel);
        if (p.empty())
            continue;
        sf::SoundBuffer b;
        if (!b.loadFromFile(p))
            continue;
        m_shotVariants.push_back(std::move(b));
    }
    if (static_cast<int>(m_shotVariants.size()) >= kGunShotMinVariants)
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
        (void)tryLoad(i, kRelPaths[i]);
    }

    /// Fallbacks voor modded/custom-bestanden naast primaire MP3 in `kRelPaths`.
    {
        const int wi = static_cast<int>(Sfx::Warp);
        auto loadResolved = [&](const char* rel) -> bool {
            const auto p = resolvedFilePathIfExists(rel);
            if (p.empty())
                return false;
            return m_buf[wi].loadFromFile(p);
        };
        if (!m_ok[wi])
            m_ok[wi] = loadResolved("assets/sounds/warp.wav");
        if (!m_ok[wi])
            m_ok[wi] = loadResolved("assets/sounds/warp.ogg");
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

    // Chest: eigen .wav in assets/sounds; ontbreekt → plinko_drop / plinko_score
    {
        const int openI = static_cast<int>(Sfx::ChestOpen);
        if (!m_ok[openI] && m_ok[static_cast<int>(Sfx::PlinkoDrop)]) {
            m_buf[openI] = m_buf[static_cast<int>(Sfx::PlinkoDrop)];
            m_ok[openI]  = true;
        }
        const int lootI = static_cast<int>(Sfx::ChestLoot);
        if (!m_ok[lootI] && m_ok[static_cast<int>(Sfx::PlinkoScore)]) {
            m_buf[lootI] = m_buf[static_cast<int>(Sfx::PlinkoScore)];
            m_ok[lootI]  = true;
        } else if (!m_ok[lootI] && m_ok[static_cast<int>(Sfx::OreCollect)]) {
            m_buf[lootI] = m_buf[static_cast<int>(Sfx::OreCollect)];
            m_ok[lootI]  = true;
        }
        const int lvI = static_cast<int>(Sfx::LevelUp);
        if (!m_ok[lvI] && m_ok[openI]) {
            m_buf[lvI] = m_buf[openI];
            m_ok[lvI]  = true;
        } else if (!m_ok[lvI] && m_ok[static_cast<int>(Sfx::PlinkoScore)]) {
            m_buf[lvI] = m_buf[static_cast<int>(Sfx::PlinkoScore)];
            m_ok[lvI]  = true;
        }
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

    for (int i = 0; i < kPool; ++i) {
        m_pool[i].emplace(m_buf[fb]);
        if (m_pool[i])
            m_pool[i]->setRelativeToListener(true);
    }

    m_ready = true;

    m_bossMusicFileOk =
        openMusicFromAsset(m_bossMusic, "assets/sounds/bossmusic.mp3")
        || openMusicFromAsset(m_bossMusic, "assets/sounds/bossmusic.ogg");
    m_gameOverMusicFileOk =
        openMusicFromAsset(m_gameOverMusic, "assets/sounds/gameover.mp3")
        || openMusicFromAsset(m_gameOverMusic, "assets/sounds/gameover.wav");
    m_menuMusicFileOk =
        openMusicFromAsset(m_menuMusic, "assets/sounds/traploop.mp3")
        || openMusicFromAsset(m_menuMusic, "assets/sounds/traploop.ogg");

    namespace fs = std::filesystem;
    m_miningTrackPaths.clear();
    for (int i = 1; i <= 9; ++i) {
        char rel[112];
        std::snprintf(rel, sizeof rel,
                      "assets/sounds/Lightyear City (%d).ogg", i);
        const std::string abs = resolveAssetPath(rel);
        std::error_code ec;
        if (fs::is_regular_file(fs::path(abs), ec))
            m_miningTrackPaths.push_back(fs::path(abs));
    }
    if (!m_miningTrackPaths.empty())
        m_miningTrackIndex = randInt(
            0, static_cast<int>(m_miningTrackPaths.size()) - 1);

    applyMusicVolumes();
}

void SoundHub::setMuted(bool m) {
    m_muted = m;
    applyMusicVolumes();
}

void SoundHub::applyMusicVolumes() {
    const float g = m_muted ? 0.f : 1.f;
    if (m_bossMusicFileOk)
        m_bossMusic.setVolume(38.f * g);
    if (m_gameOverMusicFileOk)
        m_gameOverMusic.setVolume(std::min(88.f, m_masterVol * 0.95f) * g);
    if (m_menuMusicFileOk)
        m_menuMusic.setVolume(34.f * g);
}

void SoundHub::syncBossMusic(bool bossAlive) {
    applyMusicVolumes();
    if (!m_bossMusicFileOk)
        return;
    if (!bossAlive) {
        m_bossMusic.stop();
        return;
    }
    if (!m_ready)
        return;
    if (m_gameOverMusicFileOk
        && m_gameOverMusic.getStatus() == sf::SoundSource::Status::Playing)
        return;

    m_bossMusic.setLooping(true);
    m_bossMusic.setRelativeToListener(true);
    if (m_bossMusic.getStatus() != sf::SoundSource::Status::Playing)
        m_bossMusic.play();
    m_bossMusic.setVolume(m_muted ? 0.f : 46.f);
}

bool SoundHub::playGameOverMusicOnce() {
    if (m_bossMusicFileOk)
        m_bossMusic.stop();
    if (m_menuMusicFileOk)
        m_menuMusic.stop();
    m_miningMusic.stop();
    m_miningSessionActive = false;
    m_miningPausedForBoss = false;
    if (!m_ready || !m_gameOverMusicFileOk)
        return false;
    m_gameOverMusic.stop();
    m_gameOverMusic.setLooping(false);
    m_gameOverMusic.setRelativeToListener(true);
    applyMusicVolumes();
    if (m_muted)
        return false;
    m_gameOverMusic.play();
    return true;
}

void SoundHub::stopGameOverMusic() {
    if (m_gameOverMusicFileOk)
        m_gameOverMusic.stop();
}

void SoundHub::syncMiningAmbientMusic(bool miningTabActive, bool bossAlive) {
    if (!m_ready || m_miningTrackPaths.empty()) {
        m_miningMusic.stop();
        m_miningSessionActive = false;
        m_miningPausedForBoss = false;
        return;
    }

    const float g = m_muted ? 0.f : 1.f;
    constexpr float kMiningBgVol = 15.f;

    if (!miningTabActive) {
        m_miningMusic.stop();
        m_miningSessionActive = false;
        m_miningPausedForBoss = false;
        if (!m_miningTrackPaths.empty())
            m_miningTrackIndex = randInt(
                0, static_cast<int>(m_miningTrackPaths.size()) - 1);
        return;
    }

    if (bossAlive) {
        if (m_miningMusic.getStatus() == sf::SoundSource::Status::Playing)
            m_miningMusic.pause();
        m_miningPausedForBoss = true;
        return;
    }

    if (m_miningPausedForBoss) {
        if (m_miningMusic.getStatus() == sf::SoundSource::Status::Paused)
            m_miningMusic.play();
        m_miningPausedForBoss = false;
    }

    m_miningMusic.setRelativeToListener(true);
    m_miningMusic.setLooping(false);

    const int n = static_cast<int>(m_miningTrackPaths.size());
    if (m_miningMusic.getStatus() == sf::SoundSource::Status::Stopped) {
        if (!m_miningSessionActive)
            m_miningSessionActive = true;
        else
            m_miningTrackIndex = (m_miningTrackIndex + 1) % n;

        if (!m_miningMusic.openFromFile(
                m_miningTrackPaths[static_cast<size_t>(m_miningTrackIndex)]))
            return;
        m_miningMusic.setLooping(false);
        m_miningMusic.setVolume(kMiningBgVol * g);
        if (!m_muted)
            m_miningMusic.play();
    } else
        m_miningMusic.setVolume(kMiningBgVol * g);
}

void SoundHub::syncMainMenuMusic(bool showMainMenu) {
    applyMusicVolumes();
    if (!m_menuMusicFileOk)
        return;
    if (!showMainMenu) {
        m_menuMusic.stop();
        return;
    }
    if (!m_ready)
        return;

    // Main menu gebruikt altijd traploop als enige achtergrondmuziek.
    if (m_bossMusicFileOk)
        m_bossMusic.stop();
    if (m_gameOverMusicFileOk)
        m_gameOverMusic.stop();
    m_miningMusic.stop();
    m_miningSessionActive = false;
    m_miningPausedForBoss = false;

    m_menuMusic.setLooping(true);
    m_menuMusic.setRelativeToListener(true);
    if (m_menuMusic.getStatus() != sf::SoundSource::Status::Playing) {
        m_menuMusic.setPlayingOffset(sf::Time::Zero);
        m_menuMusic.play();
    }
}

void SoundHub::stopWarpSound() {
    if (m_warpSound && m_warpSound->getStatus() == sf::SoundSource::Status::Playing)
        m_warpSound->stop();
}

void SoundHub::play(Sfx id, float warpPitch) {
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
        m_warpSound->setPitch(std::max(0.25f, warpPitch));
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
    else if (id == Sfx::ChestOpen)
        vol = std::min(100.f, m_masterVol * 1.08f);
    else if (id == Sfx::ChestLoot)
        vol = std::min(100.f, m_masterVol * 0.92f);
    else if (id == Sfx::LevelUp)
        vol = std::min(100.f, m_masterVol * 0.88f);
    ch->setVolume(vol);
    float pitch = 1.f;
    if (id == Sfx::UiClick)
        pitch = 0.94f + 0.08f * static_cast<float>(std::fmod(t * 17.0, 1.0));
    else if (id == Sfx::Shot && m_shotVariants.empty())
        pitch = 0.94f + 0.08f * static_cast<float>(std::fmod(t * 17.0, 1.0));
    else if (id == Sfx::ChestOpen)
        pitch = 0.94f;
    else if (id == Sfx::ChestLoot)
        pitch = 1.05f;
    else if (id == Sfx::LevelUp)
        pitch = 1.f;
    ch->setPitch(pitch);
    ch->play();
}
