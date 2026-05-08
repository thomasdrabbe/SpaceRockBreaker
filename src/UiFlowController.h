#pragma once
#include "Constants.h"

class NotificationSystem;

class UiFlowController {
public:
    UiFlowController(Tab& activeTab, NotificationSystem& notifications);

    void activateTab(Tab t);

private:
    Tab& m_activeTab;
    NotificationSystem& m_notifications;
};
