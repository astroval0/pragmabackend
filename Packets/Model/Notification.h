#pragma once
#include <SpectreWebsocket.h>
#include <SpectreRpcType.h>

class Notification {
private:
	SpectreRpcType m_notificationType;
public:
    virtual ~Notification() = default;
    explicit Notification(SpectreRpcType notificationType);

	const SpectreRpcType& GetNotificationType() const;
	virtual void SendTo(SpectreWebsocket& sock) const = 0;
};