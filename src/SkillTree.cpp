#include "SkillTree.h"
#include "GameState.h"
#include "Utils.h"
#include <algorithm>
#include <sstream>

const std::vector<UpgradeNodeDef> SKILL_TREE_NODES = {
    { UpgradeID::GUN_DAMAGE,    0, 0 },
    { UpgradeID::ORE_VALUE,     0, 2 },
    { UpgradeID::PLINKO_BALLS,  0, 4 },

    { UpgradeID::FIRE_RATE,     1, 0, UpgradeID::GUN_DAMAGE,    1 },
    { UpgradeID::SPLIT_SHOT,    2, 0, UpgradeID::FIRE_RATE,     3 },
    { UpgradeID::CRIT_CHANCE,   3, 0, UpgradeID::SPLIT_SHOT,    1 },
    { UpgradeID::CRIT_MULT,     4, 0, UpgradeID::CRIT_CHANCE,   3 },
    { UpgradeID::TURRET_COUNT,  3, 1, UpgradeID::SPLIT_SHOT,    1 },
    { UpgradeID::TURRET_DAMAGE, 4, 1, UpgradeID::TURRET_COUNT,  1 },
    { UpgradeID::TURRET_FIRE_RATE, 5, 1, UpgradeID::TURRET_DAMAGE, 2 },
    { UpgradeID::TURRET_FUEL_DRAIN, 6, 1, UpgradeID::TURRET_FIRE_RATE, 2 },
    { UpgradeID::TARGET_PRIORITY, 7, 1, UpgradeID::TURRET_COUNT,  2 },
    { UpgradeID::SEEKING_BULLETS, 8, 1, UpgradeID::TARGET_PRIORITY, 2 },
    { UpgradeID::SATELLITE,       9, 1, UpgradeID::TURRET_FUEL_DRAIN, 2 },
    { UpgradeID::BULLET_RANGE,  4, 2, UpgradeID::TURRET_COUNT,  3 },

    { UpgradeID::AUTO_COLLECT_RADIUS, 1, 2, UpgradeID::ORE_VALUE, 1 },
    { UpgradeID::ORE_LUCK,      2, 2, UpgradeID::AUTO_COLLECT_RADIUS, 3 },
    { UpgradeID::ORE_ON_KILL,   2, 1, UpgradeID::ORE_LUCK,      2 },
    { UpgradeID::ASTEROID_HP,   2, 3, UpgradeID::ORE_VALUE,     3 },
    { UpgradeID::EXPLOSIVE_ASTEROIDS, 5, 7, UpgradeID::ASTEROID_HP, 3 },
    { UpgradeID::CHAIN_REACTION, 6, 7, UpgradeID::EXPLOSIVE_ASTEROIDS, 3 },
    { UpgradeID::WARP_DRIVE,    1, 3, UpgradeID::ORE_VALUE,     1 },
    { UpgradeID::SHIP_SPEED,    0, 3, UpgradeID::WARP_DRIVE,    1 },
    { UpgradeID::SPEED_EFFICIENCY, 0, 5, UpgradeID::SHIP_SPEED,  3 },
    { UpgradeID::WARP_ORE_BONUS,  1, 5, UpgradeID::WARP_DRIVE,    3 },
    { UpgradeID::AUTO_WARP,       2, 5, UpgradeID::WARP_DRIVE,    5 },

    { UpgradeID::UNLOCK_BRONZE,  3, 2, UpgradeID::ORE_VALUE,      3 },
    { UpgradeID::UNLOCK_SILVER,  4, 2, UpgradeID::UNLOCK_BRONZE,  1 },
    { UpgradeID::UNLOCK_GOLD,    5, 2, UpgradeID::UNLOCK_SILVER,  1 },
    { UpgradeID::UNLOCK_DIAMOND, 6, 2, UpgradeID::UNLOCK_GOLD,    1 },
    { UpgradeID::UNLOCK_PLATINUM,7, 2, UpgradeID::UNLOCK_DIAMOND, 1 },
    { UpgradeID::UNLOCK_TITANIUM,8, 2, UpgradeID::UNLOCK_PLATINUM,1 },
    { UpgradeID::UNLOCK_IRIDIUM, 9, 2, UpgradeID::UNLOCK_TITANIUM,1 },

    { UpgradeID::AUTO_PLINKO,    3, 4, UpgradeID::PLINKO_BALLS,    5 },
    { UpgradeID::AUTO_SELL_THRESHOLD, 4, 4, UpgradeID::AUTO_PLINKO, 3 },
    { UpgradeID::PLINKO_ROWS,    2, 5, UpgradeID::PLINKO_BALLS,    3 },

    { UpgradeID::CREDIT_MULT,    3, 3, UpgradeID::PLINKO_BALLS,    1 },
    { UpgradeID::BULK_PROCESS,   4, 3, UpgradeID::CREDIT_MULT,    3 },

    { UpgradeID::METEOR_DAMAGE,  5, 0, UpgradeID::SPLIT_SHOT,     1 },
    { UpgradeID::METEOR_SIZE,    6, 0, UpgradeID::METEOR_DAMAGE,  3 },

    { UpgradeID::FUEL_CAPACITY,  5, 5, UpgradeID::WARP_DRIVE,     1 },
    { UpgradeID::FUEL_EFFICIENCY, 6, 5, UpgradeID::FUEL_CAPACITY, 1 },
    { UpgradeID::FUEL_ON_KILL,   5, 6, UpgradeID::FUEL_CAPACITY, 1 },
    { UpgradeID::SHIELD_HP,       4, 6, UpgradeID::FUEL_ON_KILL,  1 },
    { UpgradeID::SHIELD_RECHARGE,  5, 7, UpgradeID::SHIELD_HP,     2 },
    { UpgradeID::SHIELD_DELAY,    6, 7, UpgradeID::SHIELD_RECHARGE, 2 },
    { UpgradeID::SHIELD_MULTI_HIT, 7, 7, UpgradeID::SHIELD_DELAY, 3 },
    { UpgradeID::FUEL_ON_PICKUP, 4, 6, UpgradeID::AUTO_COLLECT_RADIUS, 1 },
    { UpgradeID::FUEL_WARP_REFILL, 2, 4, UpgradeID::WARP_DRIVE, 1 },
};

namespace {

const UpgradeNodeDef* findNodeById(UpgradeID id) {
    for (const auto& n : SKILL_TREE_NODES) {
        if (n.id == id)
            return &n;
    }
    return nullptr;
}

} // namespace

void SkillTreeScreen::init(sf::Font& font,
                            float panelX, float panelY,
                            float panelW, float panelH,
                            float uiScale) {
    m_font    = &font;
    m_x       = panelX;
    m_y       = panelY;
    m_w       = panelW;
    m_h       = panelH;
    m_uiScale = std::max(0.75f, uiScale);
    resetScroll();
}

void SkillTreeScreen::resetScroll() {
    m_scrollX = 0.f;
    m_scrollY = 0.f;
    updateScrollLimits();
}

void SkillTreeScreen::updateScrollLimits() const {
    int maxGX = 0;
    int maxGY = 0;
    for (const auto& n : SKILL_TREE_NODES) {
        maxGX = std::max(maxGX, n.gridX);
        maxGY = std::max(maxGY, n.gridY);
    }
    const float contentW =
        panelPad() + static_cast<float>(maxGX) * gridStepX() + nodeW() + panelPad();
    const float contentH =
        panelPad() + static_cast<float>(maxGY) * gridStepY() + nodeH() + panelPad();
    m_minScrollX = std::min(0.f, m_w - contentW);
    m_minScrollY = std::min(0.f, m_h - contentH);
}

sf::Vector2f SkillTreeScreen::nodeScreenPos(const UpgradeNodeDef& node) const {
    return {
        m_x + panelPad() + static_cast<float>(node.gridX) * gridStepX() + m_scrollX,
        m_y + panelPad() + static_cast<float>(node.gridY) * gridStepY() + m_scrollY,
    };
}

sf::FloatRect SkillTreeScreen::nodeRect(const UpgradeNodeDef& node) const {
    const auto pos = nodeScreenPos(node);
    return { pos, { nodeW(), nodeH() } };
}

void SkillTreeScreen::draw(sf::RenderTarget& target,
                            const GameState&  state,
                            bool              seeThroughMiningBackdrop) const {
    updateScrollLimits();

    sf::RectangleShape bg({ m_w, m_h });
    bg.setPosition({ m_x, m_y });
    bg.setFillColor(hubBackdropTint(sf::Color(10, 12, 24, 255),
                                    seeThroughMiningBackdrop));
    target.draw(bg);

    m_hoveredNode = nullptr;

    drawConnections(target, state);

    for (const auto& node : SKILL_TREE_NODES) {
        if (!state.isNodeVisible(node))
            continue;
        drawNode(target, node, state, seeThroughMiningBackdrop);
    }

    if (m_hoveredNode)
        drawTooltip(target, *m_hoveredNode, state, nodeScreenPos(*m_hoveredNode));
}

void SkillTreeScreen::drawConnections(sf::RenderTarget& target,
                                       const GameState&  state) const {
    for (const auto& node : SKILL_TREE_NODES) {
        if (!state.isNodeVisible(node))
            continue;
        if (node.requireId == UpgradeID::UPGRADE_COUNT)
            continue;

        const UpgradeNodeDef* parent = findNodeById(node.requireId);
        if (!parent || !state.isNodeVisible(*parent))
            continue;

        sf::Vector2f from = nodeScreenPos(*parent);
        from.x += nodeW() * 0.5f;
        from.y += nodeH() * 0.5f;

        sf::Vector2f to = nodeScreenPos(node);
        to.x += nodeW() * 0.5f;
        to.y += nodeH() * 0.5f;

        const bool unlocked = state.isNodeUnlocked(node);
        const sf::Color lineColor = unlocked
            ? sf::Color(80, 160, 80, 200)
            : sf::Color(60, 60, 80, 140);

        sf::Vertex line[2];
        line[0].position = from;
        line[0].color    = lineColor;
        line[1].position = to;
        line[1].color    = lineColor;
        target.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void SkillTreeScreen::drawNode(sf::RenderTarget&     target,
                                const UpgradeNodeDef& node,
                                const GameState&      state,
                                bool seeThroughMiningBackdrop) const {
    const auto rect = nodeRect(node);
    const auto pos  = nodeScreenPos(node);

    if (rect.position.x + rect.size.x < m_x || rect.position.x > m_x + m_w)
        return;
    if (rect.position.y + rect.size.y < m_y || rect.position.y > m_y + m_h)
        return;

    const bool unlocked = state.isNodeUnlocked(node);
    const bool bought   = state.levelOf(node.id) > 0;
    const auto& def =
        GameState::upgradeCatalog[static_cast<std::size_t>(
            static_cast<int>(node.id))];
    const bool maxed = def.maxLevel > 0
        && state.levelOf(node.id) >= def.maxLevel;
    const bool affordable = state.canBuy(node.id);
    const bool hovered    = rect.contains(m_mousePos);
    if (hovered)
        m_hoveredNode = &node;

    sf::Color bgColor;
    if (!unlocked)
        bgColor = sf::Color(20, 22, 35, 220);
    else if (maxed)
        bgColor = sf::Color(20, 80, 20, 230);
    else if (bought)
        bgColor = sf::Color(15, 40, 70, 230);
    else if (affordable)
        bgColor = sf::Color(20, 25, 50, 230);
    else
        bgColor = sf::Color(18, 20, 38, 230);

    sf::Color borderColor;
    if (!unlocked)
        borderColor = sf::Color(50, 50, 70);
    else if (maxed)
        borderColor = sf::Color(60, 200, 60);
    else if (hovered && affordable)
        borderColor = sf::Color(120, 180, 255);
    else if (affordable)
        borderColor = sf::Color(80, 140, 255);
    else
        borderColor = sf::Color(60, 80, 120);

    const float padIn = 8.f * m_uiScale;
    const float line1 = 8.f * m_uiScale;
    const float line2 = 28.f * m_uiScale;
    const float line3 = 46.f * m_uiScale;

    sf::RectangleShape bg({ nodeW(), nodeH() });
    bg.setPosition(pos);
    bg.setFillColor(hubBackdropTint(bgColor, seeThroughMiningBackdrop));
    bg.setOutlineColor(borderColor);
    bg.setOutlineThickness((hovered ? 2.5f : 2.f) * m_uiScale);
    target.draw(bg);

    sf::Text name(*m_font);
    name.setString(def.name);
    name.setCharacterSize(fontSz(13));
    name.setFillColor(unlocked ? sf::Color(220, 230, 255)
                               : sf::Color(80, 80, 100));
    name.setPosition({ pos.x + padIn, pos.y + line1 });
    target.draw(name);

    const int lv = state.levelOf(node.id);
    if (lv > 0) {
        sf::Text lvText(*m_font);
        lvText.setString("Lv " + std::to_string(lv));
        lvText.setCharacterSize(fontSz(12));
        lvText.setFillColor(sf::Color(120, 200, 120));
        lvText.setPosition({ pos.x + padIn, pos.y + line2 });
        target.draw(lvText);
    }

    if (node.id == UpgradeID::AUTO_PLINKO && state.autoPlinkoUnlockedByBoss()
        && lv == 0) {
        sf::Text bossLabel(*m_font);
        bossLabel.setString("Boss Reward!");
        bossLabel.setCharacterSize(fontSz(12));
        bossLabel.setStyle(sf::Text::Bold);
        bossLabel.setFillColor(sf::Color(255, 170, 0));
        bossLabel.setPosition({ pos.x + padIn, pos.y + line3 });
        target.draw(bossLabel);
        return;
    }

    if (unlocked && !maxed) {
        sf::Text priceText(*m_font);
        priceText.setString("$" + formatBig(state.costOf(node.id)));
        priceText.setCharacterSize(fontSz(12));
        priceText.setFillColor(affordable ? sf::Color(255, 215, 50)
                                          : sf::Color(120, 80, 80));
        priceText.setPosition({ pos.x + padIn, pos.y + line3 });
        target.draw(priceText);
    }

    if (!unlocked) {
        sf::Text lockText(*m_font);
        lockText.setString("[LOCKED]");
        lockText.setCharacterSize(fontSz(11));
        lockText.setFillColor(sf::Color(80, 80, 100));
        lockText.setPosition({ pos.x + padIn, pos.y + line3 });
        target.draw(lockText);

        if (const UpgradeNodeDef* req = findNodeById(node.requireId)) {
            const auto& reqDef =
                GameState::upgradeCatalog[static_cast<std::size_t>(
                    static_cast<int>(req->id))];
            std::string needs = "Needs: " + reqDef.name;
            if (node.requireLevel > 1)
                needs += " lv" + std::to_string(node.requireLevel);
            sf::Text reqText(*m_font);
            reqText.setString(needs);
            reqText.setCharacterSize(fontSz(10));
            reqText.setFillColor(sf::Color(100, 100, 130));
            reqText.setPosition({ pos.x + padIn, pos.y + nodeH() - 16.f * m_uiScale });
            target.draw(reqText);
        }
    }

    if (maxed) {
        sf::Text maxText(*m_font);
        maxText.setString("MAXED");
        maxText.setCharacterSize(fontSz(12));
        maxText.setStyle(sf::Text::Bold);
        maxText.setFillColor(sf::Color(80, 220, 80));
        maxText.setPosition({ pos.x + padIn, pos.y + line3 });
        target.draw(maxText);
    }
}

void SkillTreeScreen::drawTooltip(sf::RenderTarget&     target,
                                   const UpgradeNodeDef& node,
                                   const GameState&      state,
                                   sf::Vector2f          nodePos) const {
    const auto& def =
        GameState::upgradeCatalog[static_cast<std::size_t>(
            static_cast<int>(node.id))];

    const float tw = 220.f * m_uiScale;
    const float th = 96.f * m_uiScale;
    const float tipPad = 8.f * m_uiScale;
    float tx = nodePos.x + nodeW() + tipPad;
    float ty = nodePos.y;
    if (tx + tw > m_x + m_w)
        tx = nodePos.x - tw - tipPad;
    if (ty + th > m_y + m_h)
        ty = m_y + m_h - th - 4.f * m_uiScale;

    sf::RectangleShape bg({ tw, th });
    bg.setPosition({ tx, ty });
    bg.setFillColor(sf::Color(10, 12, 30, 240));
    bg.setOutlineColor(sf::Color(80, 100, 180));
    bg.setOutlineThickness(std::max(1.f, 1.f * m_uiScale));
    target.draw(bg);

    sf::Text title(*m_font);
    title.setString(def.name);
    title.setCharacterSize(fontSz(15));
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color(200, 220, 255));
    title.setPosition({ tx + tipPad, ty + tipPad });
    target.draw(title);

    sf::Text desc(*m_font);
    desc.setString(def.description);
    desc.setCharacterSize(fontSz(13));
    desc.setFillColor(sf::Color(160, 170, 200));
    desc.setPosition({ tx + tipPad, ty + 30.f * m_uiScale });
    target.draw(desc);

    const int lv = state.levelOf(node.id);
    std::string lvStr = "Level: " + std::to_string(lv);
    if (def.maxLevel > 0)
        lvStr += " / " + std::to_string(def.maxLevel);
    sf::Text lvInfo(*m_font);
    lvInfo.setString(lvStr);
    lvInfo.setCharacterSize(fontSz(13));
    lvInfo.setFillColor(sf::Color(120, 200, 120));
    lvInfo.setPosition({ tx + tipPad, ty + 52.f * m_uiScale });
    target.draw(lvInfo);

    sf::Text costInfo(*m_font);
    costInfo.setString("Cost: $" + formatBig(state.costOf(node.id)));
    costInfo.setCharacterSize(fontSz(13));
    costInfo.setFillColor(sf::Color(255, 215, 50));
    costInfo.setPosition({ tx + tipPad, ty + 72.f * m_uiScale });
    target.draw(costInfo);
}

bool SkillTreeScreen::handleClick(sf::Vector2f pos, GameState& state) {
    for (const auto& node : SKILL_TREE_NODES) {
        if (!state.isNodeVisible(node))
            continue;
        if (!state.isNodeUnlocked(node))
            continue;
        if (!nodeRect(node).contains(pos))
            continue;
        if (!state.canBuy(node.id))
            continue;
        state.buy(node.id);
        return true;
    }
    return false;
}

void SkillTreeScreen::handleScroll(float delta, sf::Vector2f /*pos*/) {
    updateScrollLimits();
    m_scrollY += delta * 48.f * m_uiScale;
    m_scrollY = std::clamp(m_scrollY, m_minScrollY, 40.f);
}
