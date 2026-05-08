#include "ChestScreen.h"
#include "Utils.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace {

uint64_t floatBits64(float f) {
    uint32_t u = 0;
    std::memcpy(&u, &f, sizeof(u));
    return static_cast<uint64_t>(u);
}

bool chestPoolHasRoom(const GameState& state) {
    for (int i = 0; i < static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT);
         ++i) {
        const auto& d = GameState::chestCatalog[i];
        if (d.maxLevel <= 0
            || state.levelOfChest(static_cast<ChestUpgradeID>(i)) < d.maxLevel)
            return true;
    }
    return false;
}

bool canOpenOneChest(const GameState& state) {
    return state.keys >= 1 && chestPoolHasRoom(state);
}

void drawMiniChest(sf::RenderTarget& target, sf::Vector2f c, float s,
                   const sf::Texture* chestTex, float animT, int index) {
    if (chestTex && chestTex->getSize().x > 0u) {
        const sf::Vector2u tsz = chestTex->getSize();
        const float          bob =
            std::sin(animT * 2.1f + static_cast<float>(index) * 0.58f) * (s * 0.08f);
        c.y += bob;

        sf::Sprite         spr(*chestTex);
        const sf::Vector2u fpx = chestSheetFrameSize(tsz);
        const bool         sheet = chestTexIsAnimatedSheet(tsz);
        if (sheet) {
            const unsigned idleRow = 0;
            const int      idleCol =
                static_cast<int>(animT * 1.15f + static_cast<float>(index) * 0.31f)
                % static_cast<int>(CHEST_SHEET_COLS);
            spr.setTextureRect(
                chestSheetFrameRect(fpx, static_cast<unsigned>(idleCol), idleRow));
        }
        const float tw = sheet ? static_cast<float>(fpx.x) : static_cast<float>(tsz.x);
        const float th = sheet ? static_cast<float>(fpx.y) : static_cast<float>(tsz.y);
        const float side = s * 1.05f;
        const float sc   = side / std::max(1.f, std::max(tw, th));
        spr.setOrigin({ tw * 0.5f, th * 0.5f });
        spr.setPosition(c);
        spr.setScale({ sc, sc });
        target.draw(spr);
        return;
    }
    sf::RectangleShape base(sf::Vector2f{ s * 0.72f, s * 0.5f });
    base.setOrigin({ base.getSize().x * 0.5f, base.getSize().y * 0.5f });
    base.setPosition(c + sf::Vector2f(0.f, s * 0.08f));
    base.setFillColor(sf::Color(95, 62, 38));
    base.setOutlineColor(sf::Color(210, 170, 90, 200));
    base.setOutlineThickness(1.f);
    target.draw(base);
    sf::ConvexShape lid;
    lid.setPointCount(4);
    lid.setPoint(0, { -s * 0.38f, 0.f });
    lid.setPoint(1, { 0.f, -s * 0.28f });
    lid.setPoint(2, { s * 0.38f, 0.f });
    lid.setPoint(3, { 0.f, -s * 0.12f });
    lid.setPosition(c + sf::Vector2f(0.f, -s * 0.12f));
    lid.setFillColor(sf::Color(120, 78, 48));
    lid.setOutlineColor(sf::Color(255, 215, 130, 220));
    lid.setOutlineThickness(1.f);
    target.draw(lid);
}

} // namespace

ChestScreen::ChestScreen() {}

void ChestScreen::init(sf::Font& font,
                       float panelX, float panelY,
                       float panelW, float panelH,
                       float scale,
                       const sf::Texture* chestTex) {
    m_font     = &font;
    m_x        = panelX;
    m_y        = panelY;
    m_w        = panelW;
    m_h        = panelH;
    m_scale    = scale;
    m_chestTex = chestTex;

    m_cardMargin = std::round(10.f * m_scale);
    m_cardPad    = std::round(12.f * m_scale);

    m_layoutFp = 0;
}

uint64_t ChestScreen::layoutFingerprint(const GameState& state) const {
    uint64_t h   = 1469598103934665603ull;
    auto     mix = [&](uint64_t x) {
        h ^= x + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    };

    mix(static_cast<uint64_t>(static_cast<unsigned>(
            std::max(0, state.keys))));
    for (int lv : state.chestLevels)
        mix(static_cast<uint64_t>(static_cast<unsigned>(lv)));

    mix(floatBits64(m_x));
    mix(floatBits64(m_y));
    mix(floatBits64(m_w));
    mix(floatBits64(m_h));
    mix(floatBits64(m_lootBtn.position.x));
    mix(floatBits64(m_lootBtn.position.y));
    mix(floatBits64(m_lootBtn.size.x));
    mix(floatBits64(m_lootBtn.size.y));
    mix(static_cast<uint64_t>(m_overlayPlaying ? 1u : 0u));
    mix(static_cast<uint64_t>(m_chestTex ? 1u : 0u));
    return h;
}

void ChestScreen::rebuildLayout(const GameState& state) {
    const float btnW = std::min(m_w - m_cardMargin * 2.f, 440.f);
    const float btnH = std::round(56.f * m_scale);
    const float bx   = m_x + (m_w - btnW) * 0.5f;
    const float by   = m_y + m_h - btnH - m_cardMargin - std::round(18.f * m_scale);

    m_lootBtn = sf::FloatRect({ bx, by }, { btnW, btnH });
    m_layoutFp = layoutFingerprint(state);
}

bool ChestScreen::handleEvent(const sf::Event& event, GameState& state,
                              const sf::RenderWindow& window,
                              bool chestOverlayBlocking,
                              ChestUpgradeID* outPurchased) {
    if (chestOverlayBlocking)
        return false;

    if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = mapPixelToUi(window, sf::Vector2i(e->position));

            if (m_lootBtn.contains(mp)) {
                ChestUpgradeID got{};
                if (state.openOneChest(&got)) {
                    if (outPurchased)
                        *outPurchased = got;
                    rebuildLayout(state);
                    return true;
                }
            }
        }
    }

    return false;
}

void ChestScreen::update(float dt, sf::Vector2f mousePos,
                         const GameState& state, bool chestOverlayPlaying) {
    m_chestAnimT += dt;
    m_overlayPlaying = chestOverlayPlaying;
    const uint64_t fp = layoutFingerprint(state);
    if (fp != m_layoutFp)
        rebuildLayout(state);
    m_lootHovered = m_lootBtn.contains(mousePos) && canOpenOneChest(state)
                    && !m_overlayPlaying;
}

void ChestScreen::scrollBy(float /*delta*/) {}

std::string ChestScreen::formatEffect(ChestUpgradeID id,
                                      const GameState& state) const {
    switch (id) {
        case ChestUpgradeID::PLINKO_PEG_SIZE:
            return "Peg rolls: "
                + std::to_string(state.chestPegUpgradeCount());
        case ChestUpgradeID::PLINKO_SLOT_MULT:
            return "Alle valbak-waarden x"
                + formatBig(static_cast<double>(
                      state.chestPlinkoSlotMult()));
        case ChestUpgradeID::PLINKO_DUPLICATOR_PEG:
            return "Duplicator rolls: "
                + std::to_string(state.chestDuplicatorRollCount());
        default:
            return "";
    }
}

void ChestScreen::draw(sf::RenderTarget& target,
                       const GameState& state,
                       bool               seeThroughMiningBackdrop) const {
    const bool st = seeThroughMiningBackdrop;

    drawPanelRect(
        target,
        sf::FloatRect({ m_x, m_y }, { m_w, m_h }),
        hubBackdropTint(sf::Color(10, 12, 22, 245), st),
        hubBackdropTint(sf::Color(90, 70, 40, 160), st),
        1.f);

    const float headerH = std::round(52.f * m_scale);
    unsigned    fTitle  = static_cast<unsigned>(std::round(20.f * m_scale));
    unsigned    fSub    = static_cast<unsigned>(std::round(13.f * m_scale));
    unsigned    fBody   = static_cast<unsigned>(std::round(12.f * m_scale));
    unsigned    fSmall  = static_cast<unsigned>(std::round(11.f * m_scale));

    sf::Text title(*m_font);
    title.setCharacterSize(fTitle);
    title.setString("CHESTS");
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color(255, 210, 120));
    title.setPosition({ m_x + m_cardMargin, m_y + 10.f });
    target.draw(title);

    std::ostringstream ks;
    ks << "Keys: " << state.keys
       << "  |  elke chest kost altijd precies 1 key";
    sf::Text sub(*m_font);
    sub.setCharacterSize(fSub);
    sub.setString(ks.str());
    sub.setFillColor(sf::Color(200, 190, 160));
    sub.setPosition({ m_x + m_cardMargin, m_y + 10.f + fTitle + 2.f });
    target.draw(sub);

    sf::RectangleShape sep(sf::Vector2f{ m_w - m_cardMargin * 2.f, 1.f });
    sep.setPosition({ m_x + m_cardMargin, m_y + headerH - 6.f });
    sep.setFillColor(hubBackdropTint(sf::Color(80, 65, 40, 200), st));
    target.draw(sep);

    float ty = m_y + headerH + m_cardMargin;

    {
        const int nk = std::max(0, state.keys);
        const int show = std::min(nk, 28);
        const float sp = std::round(30.f * m_scale);
        float       x0 = m_x + m_cardMargin;
        const float yc = ty + sp * 0.55f;
        for (int i = 0; i < show; ++i)
            drawMiniChest(target, { x0 + sp * 0.5f + i * sp, yc }, sp * 0.85f,
                          m_chestTex, m_chestAnimT, i);
        if (nk > show) {
            sf::Text more(*m_font);
            more.setCharacterSize(fSmall);
            more.setString("+" + std::to_string(nk - show));
            more.setFillColor(sf::Color(180, 170, 140));
            more.setPosition({ x0 + sp * 0.5f + show * sp + 4.f, yc - fSmall * 0.35f });
            target.draw(more);
        }
        ty += sp * 1.15f + std::round(8.f * m_scale);
    }

    sf::Text costNote(*m_font);
    costNote.setCharacterSize(fBody);
    costNote.setString(
        "Niveaus hieronder = upgrade-voortgang (geen stijgende key-prijs).");
    costNote.setFillColor(sf::Color(140, 165, 210));
    costNote.setPosition({ m_x + m_cardMargin, ty });
    target.draw(costNote);
    ty += fBody + std::round(10.f * m_scale);

    sf::Text hint(*m_font);
    hint.setCharacterSize(fBody);
    if (!chestPoolHasRoom(state)) {
        hint.setString("Alle chest-bonussen zijn op maximum.");
        hint.setFillColor(sf::Color(180, 140, 120));
    } else if (state.keys < 1) {
        hint.setString("Geen keys - versla de sleutel-asteroide in een run.");
        hint.setFillColor(sf::Color(180, 150, 130));
    } else {
        hint.setString("Willekeurige permanente bonus (na prestige behouden).");
        hint.setFillColor(sf::Color(160, 175, 210));
    }
    hint.setPosition({ m_x + m_cardMargin, ty });
    target.draw(hint);
    ty += fBody + std::round(14.f * m_scale);

    for (int i = 0; i < static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT);
         ++i) {
        const auto    id  = static_cast<ChestUpgradeID>(i);
        const auto&   def = GameState::chestCatalog[i];
        const int     lv  = state.levelOfChest(id);
        const bool    maxed =
            def.maxLevel > 0 && lv >= def.maxLevel;

        std::ostringstream line;
        line << def.name << "  |  niveau " << lv;
        if (def.maxLevel > 0)
            line << " / " << def.maxLevel;
        if (maxed)
            line << "  (MAX)";
        std::string fx = formatEffect(id, state);
        if (!fx.empty())
            line << "  |  " << fx;

        sf::Text row(*m_font);
        row.setCharacterSize(fSmall);
        row.setString(line.str());
        row.setFillColor(maxed ? sf::Color(110, 115, 135)
                               : sf::Color(190, 200, 225));
        row.setPosition({ m_x + m_cardMargin, ty });
        target.draw(row);
        ty += fSmall + std::round(4.f * m_scale);

        sf::Text desc(*m_font);
        desc.setCharacterSize(static_cast<unsigned>(
            std::max(10, static_cast<int>(fSmall) - 1)));
        desc.setString(def.description);
        desc.setFillColor(sf::Color(130, 140, 165));
        desc.setPosition({ m_x + m_cardMargin + std::round(8.f * m_scale), ty });
        target.draw(desc);
        ty += fSmall + std::round(6.f * m_scale);
    }

    const bool canLoot = canOpenOneChest(state) && !m_overlayPlaying;
    sf::RectangleShape btn(sf::Vector2f{ m_lootBtn.size.x, m_lootBtn.size.y });
    btn.setPosition(m_lootBtn.position);
    btn.setFillColor(hubBackdropTint(
        canLoot && m_lootHovered ? sf::Color(70, 55, 30, 250)
                                 : sf::Color(35, 30, 22, 245),
        st));
    btn.setOutlineColor(hubBackdropTint(
        canLoot ? sf::Color(255, 200, 90, 230)
                : sf::Color(70, 65, 55, 120),
        st));
    btn.setOutlineThickness(canLoot && m_lootHovered ? 2.5f : 1.5f);
    target.draw(btn);

    sf::Text lootLbl(*m_font);
    lootLbl.setCharacterSize(static_cast<unsigned>(std::round(17.f * m_scale)));
    lootLbl.setStyle(sf::Text::Bold);
    if (m_overlayPlaying)
        lootLbl.setString("Even geduld…");
    else if (canLoot)
        lootLbl.setString("OPEN CHEST  (1 key)");
    else if (!chestPoolHasRoom(state))
        lootLbl.setString("Geen chests meer");
    else
        lootLbl.setString("Geen keys");
    lootLbl.setFillColor(canLoot ? sf::Color(255, 235, 160)
                                 : sf::Color(120, 115, 105));
    auto lb = lootLbl.getLocalBounds();
    lootLbl.setPosition({
        m_lootBtn.position.x + (m_lootBtn.size.x - lb.size.x) * 0.5f - lb.position.x,
        m_lootBtn.position.y + (m_lootBtn.size.y - lb.size.y) * 0.5f - lb.position.y });
    target.draw(lootLbl);
}
