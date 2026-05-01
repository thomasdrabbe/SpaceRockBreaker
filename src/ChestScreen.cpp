#include "ChestScreen.h"
#include "Utils.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

ChestScreen::ChestScreen() {}

void ChestScreen::init(sf::Font& font,
                       float panelX, float panelY,
                       float panelW, float panelH,
                       float scale) {
    m_font  = &font;
    m_x     = panelX;
    m_y     = panelY;
    m_w     = panelW;
    m_h     = panelH;
    m_scale = scale;

    m_headerH    = std::round(52.f  * m_scale);
    m_cardH      = std::round(96.f  * m_scale);
    m_cardMargin = std::round(10.f  * m_scale);
    m_cardPad    = std::round(12.f  * m_scale);
    m_scrollBarW = std::round(8.f   * m_scale);
}

void ChestScreen::buildCards(const GameState& state) {
    m_cards.clear();

    float cardW = m_w - m_scrollBarW - m_cardMargin * 2.f;
    float y     = m_y + m_headerH + m_cardMargin - m_scroll;

    for (int i = 0; i < static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT);
         ++i) {
        ChestCard c;
        c.id         = static_cast<ChestUpgradeID>(i);
        c.bounds     = sf::FloatRect(
            { m_x + m_cardMargin, y },
            { cardW, m_cardH });
        c.affordable = state.canBuyChest(c.id);
        m_cards.push_back(c);
        y += m_cardH + m_cardMargin;
    }

    float totalH   = static_cast<float>(m_cards.size())
                     * (m_cardH + m_cardMargin);
    float visibleH = m_h - m_headerH;
    m_maxScroll    = std::max(0.f, totalH - visibleH);
    m_scroll       = clamp(m_scroll, 0.f, m_maxScroll);
}

bool ChestScreen::handleEvent(const sf::Event& event, GameState& state,
                               const sf::RenderWindow& window) {
    if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = mapPixelToUi(window, sf::Vector2i(e->position));

            for (auto& card : m_cards) {
                if (card.bounds.contains(mp) && card.affordable) {
                    state.buyChest(card.id);
                    buildCards(state);
                    return true;
                }
            }
        }
    }

    return false;
}

void ChestScreen::update(sf::Vector2f mousePos, const GameState& state) {
    buildCards(state);
    for (auto& card : m_cards)
        card.hovered = card.bounds.contains(mousePos);
}

void ChestScreen::scrollBy(float delta) {
    m_scroll = clamp(m_scroll + delta, 0.f, m_maxScroll);
}

std::string ChestScreen::formatEffect(ChestUpgradeID id,
                                       const GameState& state) const {
    switch (id) {
        case ChestUpgradeID::PLINKO_PEG_SIZE:
            return "Peg rarity rolls: "
                + std::to_string(state.chestPegUpgradeCount())
                + " (lvl x3)";
        case ChestUpgradeID::PLINKO_PEG_BOUNCE:
            return "Bounce mult: "
                + formatBig(static_cast<double>(
                      state.chestPlinkoBounceMult()))
                + "x";
        case ChestUpgradeID::GUN_FLAT_DAMAGE:
            return "Flat dmg: +"
                + formatBig(static_cast<double>(
                      state.chestGunFlatBonus()));
        case ChestUpgradeID::MINING_ORE_VALUE:
            return "Ore mult: "
                + formatBig(static_cast<double>(
                      state.chestOreValueMult()))
                + "x";
        default:
            return "";
    }
}

void ChestScreen::drawScrollBar(sf::RenderTarget& target) const {
    if (m_maxScroll <= 0.f) return;

    float trackX = m_x + m_w - m_scrollBarW - 4.f;
    float trackY = m_y + m_headerH;
    float trackH = m_h - m_headerH;

    sf::RectangleShape track({ m_scrollBarW, trackH });
    track.setPosition({ trackX, trackY });
    track.setFillColor(sf::Color(20, 22, 40, 200));
    target.draw(track);

    float thumbH = std::max(24.f, trackH * (trackH / (trackH + m_maxScroll)));
    float t      = (m_maxScroll > 0.f) ? (m_scroll / m_maxScroll) : 0.f;
    float thumbY = trackY + (trackH - thumbH) * t;

    sf::RectangleShape thumb({ m_scrollBarW - 2.f, thumbH });
    thumb.setPosition({ trackX + 1.f, thumbY });
    thumb.setFillColor(sf::Color(200, 170, 90, 200));
    target.draw(thumb);
}

void ChestScreen::drawCard(sf::RenderTarget& target,
                            const ChestCard&   card,
                            const GameState&   state) const {
    const int         idx = static_cast<int>(card.id);
    const auto&       def = GameState::chestCatalog[idx];
    const int         lv  = state.levelOfChest(card.id);
    const bool        maxed = def.maxLevel > 0 && lv >= def.maxLevel;

    sf::Color accent(220, 180, 90);
    bool      hov = card.hovered;
    bool      aff = card.affordable && !maxed;

    sf::RectangleShape bg(
        sf::Vector2f{ card.bounds.size.x, card.bounds.size.y });
    bg.setPosition(card.bounds.position);
    bg.setFillColor(hov && aff
        ? sf::Color(accent.r / 6, accent.g / 6, accent.b / 6, 240)
        : sf::Color(16, 18, 32, 230));
    bg.setOutlineColor(maxed
        ? sf::Color(60, 70, 90, 120)
        : (aff ? sf::Color(accent.r, accent.g, accent.b, hov ? 220 : 130)
                 : sf::Color(50, 50, 70, 100)));
    bg.setOutlineThickness(hov && aff ? 2.f : 1.f);
    target.draw(bg);

    unsigned fName = static_cast<unsigned>(std::round(15.f * m_scale));
    unsigned fDesc = static_cast<unsigned>(std::round(12.f * m_scale));
    unsigned fCost = static_cast<unsigned>(std::round(13.f * m_scale));
    unsigned fLvl  = static_cast<unsigned>(std::round(12.f * m_scale));

    float tx = card.bounds.position.x + m_cardPad;
    float ty = card.bounds.position.y + m_cardPad;

    sf::Text name(*m_font);
    name.setCharacterSize(fName);
    name.setString(def.name);
    name.setFillColor(maxed ? sf::Color(120, 120, 140)
                            : sf::Color(240, 230, 200));
    name.setStyle(sf::Text::Bold);
    name.setPosition({ tx, ty });
    target.draw(name);

    std::string lvStr =
        maxed ? ("MAX  (" + std::to_string(lv) + ")")
              : ("Lv " + std::to_string(lv));
    sf::Text lvTxt(*m_font);
    lvTxt.setCharacterSize(fLvl);
    lvTxt.setString(lvStr);
    lvTxt.setFillColor(sf::Color(accent.r, accent.g, accent.b,
                                 aff ? 220 : 140));
    lvTxt.setPosition({
        card.bounds.position.x + card.bounds.size.x - m_cardPad
            - lvTxt.getLocalBounds().size.x - 4.f,
        ty + 2.f });
    target.draw(lvTxt);

    ty += fName + std::round(6.f * m_scale);

    sf::Text desc(*m_font);
    desc.setCharacterSize(fDesc);
    desc.setString(def.description);
    desc.setFillColor(sf::Color(140, 150, 180));
    desc.setPosition({ tx, ty });
    target.draw(desc);

    ty += fDesc + std::round(6.f * m_scale);

    std::string fx = formatEffect(card.id, state);
    if (!fx.empty()) {
        sf::Text fxTxt(*m_font);
        fxTxt.setCharacterSize(fDesc);
        fxTxt.setString(fx);
        fxTxt.setFillColor(sf::Color(
            accent.r, accent.g, accent.b, aff ? 200 : 100));
        fxTxt.setPosition({ tx, ty });
        target.draw(fxTxt);
        ty += fDesc + std::round(4.f * m_scale);
    }

    std::ostringstream cs;
    if (maxed)
        cs << "— max —";
    else {
        int k = state.keyCostOf(card.id);
        cs << k << " key" << (k == 1 ? "" : "s");
    }
    sf::Text costTxt(*m_font);
    costTxt.setCharacterSize(fCost);
    costTxt.setString(cs.str());
    costTxt.setStyle(sf::Text::Bold);
    costTxt.setFillColor(aff ? sf::Color(255, 230, 150)
                             : sf::Color(100, 95, 80));
    costTxt.setPosition({
        card.bounds.position.x + card.bounds.size.x - m_cardPad
            - costTxt.getLocalBounds().size.x - 4.f,
        card.bounds.position.y + card.bounds.size.y - m_cardPad
            - static_cast<float>(fCost) });
    target.draw(costTxt);
}

void ChestScreen::draw(sf::RenderTarget& target,
                        const GameState& state) const {
    sf::RectangleShape panel(sf::Vector2f{ m_w, m_h });
    panel.setPosition({ m_x, m_y });
    panel.setFillColor(sf::Color(10, 12, 22, 245));
    panel.setOutlineColor(sf::Color(90, 70, 40, 160));
    panel.setOutlineThickness(1.f);
    target.draw(panel);

    unsigned fTitle = static_cast<unsigned>(std::round(20.f * m_scale));
    unsigned fSub  = static_cast<unsigned>(std::round(13.f * m_scale));

    sf::Text title(*m_font);
    title.setCharacterSize(fTitle);
    title.setString("CHESTS");
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color(255, 210, 120));
    title.setPosition({ m_x + m_cardMargin, m_y + 10.f });
    target.draw(title);

    std::ostringstream ks;
    ks << "Keys: " << state.keys;
    sf::Text sub(*m_font);
    sub.setCharacterSize(fSub);
    sub.setString(ks.str());
    sub.setFillColor(sf::Color(200, 190, 160));
    sub.setPosition({ m_x + m_cardMargin, m_y + 10.f + fTitle + 2.f });
    target.draw(sub);

    sf::RectangleShape sep(sf::Vector2f{ m_w - m_cardMargin * 2.f, 1.f });
    sep.setPosition({ m_x + m_cardMargin, m_y + m_headerH - 6.f });
    sep.setFillColor(sf::Color(80, 65, 40, 200));
    target.draw(sep);

    auto  tSize = target.getSize();
    float tW    = static_cast<float>(tSize.x);
    float tH    = static_cast<float>(tSize.y);

    sf::View oldView = target.getView();

    float vpX = m_x / tW;
    float vpY = (m_y + m_headerH) / tH;
    float vpW = m_w / tW;
    float vpH = (m_h - m_headerH) / tH;

    sf::View cardView(sf::FloatRect(
        { m_x, m_y + m_headerH },
        { m_w, m_h - m_headerH }));
    cardView.setViewport(sf::FloatRect({ vpX, vpY }, { vpW, vpH }));
    target.setView(cardView);

    for (const auto& card : m_cards)
        drawCard(target, card, state);

    target.setView(oldView);
    drawScrollBar(target);
}
