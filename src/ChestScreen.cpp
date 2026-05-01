#include "ChestScreen.h"
#include "Utils.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace {

uint64_t floatBits64(float f) {
    uint32_t u = 0;
    std::memcpy(&u, &f, sizeof(u));
    return static_cast<uint64_t>(u);
}

int affordableChestCount(const GameState& state) {
    int n = 0;
    for (int i = 0; i < static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT);
         ++i) {
        if (state.canBuyChest(static_cast<ChestUpgradeID>(i)))
            ++n;
    }
    return n;
}

} // namespace

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
    return h;
}

void ChestScreen::rebuildLayout(const GameState& state) {
    const float headerH = std::round(52.f * m_scale);
    const float btnW    = std::min(m_w - m_cardMargin * 2.f, 420.f);
    const float btnH    = std::round(56.f * m_scale);
    const float bx      = m_x + (m_w - btnW) * 0.5f;
    const float by      = m_y + m_h - btnH - m_cardMargin - std::round(20.f * m_scale);

    m_lootBtn = sf::FloatRect({ bx, by }, { btnW, btnH });
    m_layoutFp = layoutFingerprint(state);
}

bool ChestScreen::handleEvent(const sf::Event& event, GameState& state,
                              const sf::RenderWindow& window,
                              ChestUpgradeID* outPurchased) {
    if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = mapPixelToUi(window, sf::Vector2i(e->position));

            if (m_lootBtn.contains(mp)) {
                ChestUpgradeID got{};
                if (state.buyRandomChestUpgrade(&got)) {
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

void ChestScreen::update(sf::Vector2f mousePos, const GameState& state) {
    const uint64_t fp = layoutFingerprint(state);
    if (fp != m_layoutFp)
        rebuildLayout(state);
    m_lootHovered =
        m_lootBtn.contains(mousePos) && affordableChestCount(state) > 0;
}

void ChestScreen::scrollBy(float /*delta*/) {}

std::string ChestScreen::formatEffect(ChestUpgradeID id,
                                      const GameState& state) const {
    switch (id) {
        case ChestUpgradeID::PLINKO_PEG_SIZE:
            return "Peg rolls: "
                + std::to_string(state.chestPegUpgradeCount());
        case ChestUpgradeID::PLINKO_PEG_BOUNCE:
            return "Bounce: "
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

void ChestScreen::draw(sf::RenderTarget& target,
                       const GameState& state) const {
    sf::RectangleShape panel(sf::Vector2f{ m_w, m_h });
    panel.setPosition({ m_x, m_y });
    panel.setFillColor(sf::Color(10, 12, 22, 245));
    panel.setOutlineColor(sf::Color(90, 70, 40, 160));
    panel.setOutlineThickness(1.f);
    target.draw(panel);

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
    ks << "Keys: " << state.keys;
    sf::Text sub(*m_font);
    sub.setCharacterSize(fSub);
    sub.setString(ks.str());
    sub.setFillColor(sf::Color(200, 190, 160));
    sub.setPosition({ m_x + m_cardMargin, m_y + 10.f + fTitle + 2.f });
    target.draw(sub);

    sf::RectangleShape sep(sf::Vector2f{ m_w - m_cardMargin * 2.f, 1.f });
    sep.setPosition({ m_x + m_cardMargin, m_y + headerH - 6.f });
    sep.setFillColor(sf::Color(80, 65, 40, 200));
    target.draw(sep);

    float ty = m_y + headerH + m_cardMargin;
    const int affordN = affordableChestCount(state);

    sf::Text hint(*m_font);
    hint.setCharacterSize(fBody);
    if (affordN <= 0) {
        bool anyOpen = false;
        for (int i = 0; i < static_cast<int>(ChestUpgradeID::CHEST_UPGRADE_COUNT);
             ++i) {
            const auto id = static_cast<ChestUpgradeID>(i);
            const auto& d = GameState::chestCatalog[static_cast<int>(id)];
            if (d.maxLevel <= 0 || state.levelOfChest(id) < d.maxLevel) {
                anyOpen = true;
                break;
            }
        }
        hint.setString(anyOpen ? "Niet genoeg keys voor een upgrade."
                               : "Alle chest-upgrades zijn op max level.");
        hint.setFillColor(sf::Color(180, 140, 120));
    } else {
        hint.setString(
            "Open een chest: je krijgt een willekeurige upgrade waar je "
            "keys voor hebt (normale key-prijs).");
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
        line << def.name << "  [Lv " << lv;
        if (maxed)
            line << " MAX]";
        else
            line << "]  -  " << state.keyCostOf(id) << " keys";
        std::string fx = formatEffect(id, state);
        if (!fx.empty())
            line << "   (" << fx << ")";

        sf::Text row(*m_font);
        row.setCharacterSize(fSmall);
        row.setString(line.str());
        row.setFillColor(maxed ? sf::Color(110, 115, 135)
                               : sf::Color(190, 200, 225));
        row.setPosition({ m_x + m_cardMargin, ty });
        target.draw(row);
        ty += fSmall + std::round(5.f * m_scale);
    }

    const bool canLoot = affordN > 0;
    sf::RectangleShape btn(sf::Vector2f{ m_lootBtn.size.x, m_lootBtn.size.y });
    btn.setPosition(m_lootBtn.position);
    btn.setFillColor(canLoot && m_lootHovered
                         ? sf::Color(70, 55, 30, 250)
                         : sf::Color(35, 30, 22, 245));
    btn.setOutlineColor(canLoot ? sf::Color(255, 200, 90, 230)
                                : sf::Color(70, 65, 55, 120));
    btn.setOutlineThickness(canLoot && m_lootHovered ? 2.5f : 1.5f);
    target.draw(btn);

    sf::Text lootLbl(*m_font);
    lootLbl.setCharacterSize(static_cast<unsigned>(std::round(17.f * m_scale)));
    lootLbl.setStyle(sf::Text::Bold);
    lootLbl.setString(canLoot ? "OPEN CHEST (willekeurige upgrade)"
                              : "Geen loot beschikbaar");
    lootLbl.setFillColor(canLoot ? sf::Color(255, 235, 160)
                                 : sf::Color(120, 115, 105));
    auto lb = lootLbl.getLocalBounds();
    lootLbl.setPosition({
        m_lootBtn.position.x + (m_lootBtn.size.x - lb.size.x) * 0.5f - lb.position.x,
        m_lootBtn.position.y + (m_lootBtn.size.y - lb.size.y) * 0.5f - lb.position.y });
    target.draw(lootLbl);
}
