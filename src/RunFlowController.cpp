#include "RunFlowController.h"
#include "GameState.h"
#include "MiningScreen.h"

RunFlowController::RunFlowController(GameState& state,
                                     MiningScreen& mining,
                                     RunMode& runMode,
                                     RebuildPlinkoFn rebuildPlinko,
                                     ResetZoneKeyStateFn resetZoneKeyState)
    : m_state(state)
    , m_mining(mining)
    , m_runMode(runMode)
    , m_rebuildPlinko(std::move(rebuildPlinko))
    , m_resetZoneKeyState(std::move(resetZoneKeyState)) {}

void RunFlowController::syncFromState(bool rebuildPlinkoBoard,
                                       bool clearMiningField) {
    if (clearMiningField)
        m_mining.clearAll();
    m_mining.syncTurrets(m_state);
    if (rebuildPlinkoBoard && m_rebuildPlinko)
        m_rebuildPlinko();
}

void RunFlowController::startRun(int startZone) {
    m_state.beginRunAtZone(startZone);
    m_runMode = RunMode::RUNNING;
    m_mining.prepareNewRun();
    m_mining.initPlayerFuel(m_state);
    m_mining.syncTurrets(m_state);
    m_resetZoneKeyState();
}

void RunFlowController::moveToBase() {
    m_runMode = RunMode::BASE;
    m_state.endRunRestoreZone();
    m_resetZoneKeyState();
}

void RunFlowController::collectRunOreToState() {
    double orePick = 0.0;
    std::array<double, ORE_TIER_COUNT> oreByTier{};
    m_mining.collectAllOre(orePick, oreByTier, m_state);
    m_mining.collectAllGems(m_state);
    m_state.addOreTiered(oreByTier, true);
}
