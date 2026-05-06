#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <deque>
#include <string>
#include "Constants.h"

class NotificationSystem {
public:
    void push(const std::string& message,
              sf::Color              color    = sf::Color::Yellow,
              float                  duration = 4.f,
              int                    badgeTab = -1);

    void update(float dt);
    /// Toasts onder de tabbalk: `topInset` = Y-offset vanaf bovenkant scherm (px).
    void draw(sf::RenderTarget& target, const sf::Font& font,
              float topInset = 0.f) const;

    bool hasBadgeFor(int tabIndex) const;
    void clearBadge(int tabIndex);

    static constexpr int kMaxVisible = 5;
    static constexpr float kFadeInSec  = 0.3f;
    static constexpr float kFadeOutSec = 0.5f;

private:
    struct Entry {
        std::string message;
        sf::Color   color{ 255, 255, 100 };
        float       holdSec     = 4.f;
        float       elapsed     = 0.f;
        int         badgeTab    = -1;
        float       totalDur() const {
            return NotificationSystem::kFadeInSec + holdSec
                 + NotificationSystem::kFadeOutSec;
        }
    };

    std::deque<Entry> m_queue;
    std::array<bool, TAB_COUNT> m_tabBadges{};
};
