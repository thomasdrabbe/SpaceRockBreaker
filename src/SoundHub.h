#pragma once
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Clock.hpp>
#include <array>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

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
    ChestOpen,
    ChestLoot,
    LevelUp,
    COUNT
};

class IAudioBus {
public:
    virtual ~IAudioBus() = default;
    virtual void setMuted(bool muted) = 0;
    [[nodiscard]] virtual bool isMuted() const = 0;
    virtual void play(Sfx id, float warpPitch = 1.f) = 0;
    virtual void stopWarpSound() = 0;
    virtual void syncBossMusic(bool bossAlive) = 0;
    virtual bool playGameOverMusicOnce() = 0;
    virtual void stopGameOverMusic() = 0;
    virtual void syncMainMenuMusic(bool showMainMenu) = 0;
    virtual void syncMiningAmbientMusic(bool miningTabActive,
                                        bool bossAlive) = 0;
};

class SoundHub : public IAudioBus {
public:
    void init();
    void setMuted(bool m) override;
    bool isMuted() const override { return m_muted; }
    void play(Sfx id, float warpPitch = 1.f) override;
    /// Stop het lange warp-charge geluid (bij loslaten Space vóór warp).
    void stopWarpSound() override;

    /// Loopt zolang `bossAlive` true (zone-boss op het veld).
    void syncBossMusic(bool bossAlive) override;
    /// Eén keer bij game over; stopt boss-muziek. Retourneert false als geen file.
    bool playGameOverMusicOnce() override;
    void stopGameOverMusic() override;

    /// Hoofdmenu: `traploop` op repeat.
    void syncMainMenuMusic(bool showMainMenu) override;

    /// Mining-tab: Lightyear City-tracks (achtergrond, laag). Pauzeert tijdens boss.
    void syncMiningAmbientMusic(bool miningTabActive, bool bossAlive) override;

private:
    static constexpr int kPool = 12;

    std::array<sf::SoundBuffer, static_cast<int>(Sfx::COUNT)> m_buf{};
    std::array<bool, static_cast<int>(Sfx::COUNT)>          m_ok{};
    std::array<std::optional<sf::Sound>, kPool>            m_pool{};
    int                                                      m_next = 0;
    bool                                                     m_ready = false;
    bool                                                     m_muted = false;
    float                                                    m_masterVol = 55.f;

    /// 14 schoten: `gun sound.mp3` (gelijk opgesplitst, vaste volgorde) of
    /// `shot_01.wav`…`shot_14.wav`.
    std::vector<sf::SoundBuffer> m_shotVariants;
    int                          m_shotRing = 0;
    /// Eigen kanaal: lang warp-geluid wordt niet door sfx-pool afgekapt.
    std::optional<sf::Sound> m_warpSound;

    sf::Clock m_clk;
    float     m_lastShot   = -1.f;
    float     m_lastOre    = -1.f;
    float     m_lastExpl   = -1.f;
    float     m_lastPlinko = -1.f;

    sf::Music m_bossMusic;
    sf::Music m_gameOverMusic;
    sf::Music m_menuMusic;
    bool      m_bossMusicFileOk     = false;
    bool      m_gameOverMusicFileOk = false;
    bool      m_menuMusicFileOk     = false;

    sf::Music                       m_miningMusic;
    std::vector<std::filesystem::path> m_miningTrackPaths;
    int                             m_miningTrackIndex      = 0;
    bool                            m_miningSessionActive = false;
    bool                            m_miningPausedForBoss   = false;

    void applyMusicVolumes();

    bool tryLoad(int idx, const std::string& path);
    void loadShotVariants();
};

extern SoundHub gSfx;
