#pragma once
#include <SpectreWebsocket.h>
#include <SpectreRpcType.h>

class Notification {
private:
	SpectreRpcType m_notificationType;
public:
	Notification(SpectreRpcType notificationType);

	const SpectreRpcType& GetNotificationType() const;
	virtual void SendTo(SpectreWebsocket& sock) const = 0;
};