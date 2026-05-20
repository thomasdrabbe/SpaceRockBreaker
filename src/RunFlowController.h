#pragma once
#include <functional>
#include "Constants.h"

class GameState;
class MiningScreen;

class RunFlowController {
public:
    using ResetZoneKeyStateFn = std::function<void()>;
    using RebuildPlinkoFn = std::function<void()>;

    RunFlowController(GameState& state,
                      MiningScreen& mining,
                      RunMode& runMode,
                      RebuildPlinkoFn rebuildPlinko,
                      ResetZoneKeyStateFn resetZoneKeyState);

    void syncFromState(bool rebuildPlinkoBoard, bool clearMiningField);
    void startRun(int startZone);
    void moveToBase();
    void collectRunOreToState();

private:
    GameState& m_state;
    MiningScreen& m_mining;
    RunMode& m_runMode;
    RebuildPlinkoFn m_rebuildPlinko;
    ResetZoneKeyStateFn m_resetZoneKeyState;
};
