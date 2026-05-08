#include "UiFlowController.h"
#include "NotificationSystem.h"

UiFlowController::UiFlowController(Tab& activeTab,
                                   NotificationSystem& notifications)
    : m_activeTab(activeTab)
    , m_notifications(notifications) {}

void UiFlowController::activateTab(Tab t) {
    m_activeTab = t;
    m_notifications.clearBadge(static_cast<int>(t));
}
