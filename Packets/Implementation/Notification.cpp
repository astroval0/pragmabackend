#include <Notification.h>

Notification::Notification(SpectreRpcType notificationType) : m_notificationType(notificationType) {

}

const SpectreRpcType& Notification::GetNotificationType() const {
	return m_notificationType;
}