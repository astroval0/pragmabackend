#pragma once
#include <Notification.h>
#include <PartyDatabase.h>

class PartyDetailsNotification : public Notification {
private:
	PartyResponse payload;
public:
	PartyDetailsNotification(const std::string& partyId, SpectreRpcType notificationType);
	PartyDetailsNotification(const PartyResponse& partyRes, SpectreRpcType notificationType);
	PartyDetailsNotification(const Party& party, SpectreRpcType notificationType);

	void SendTo(SpectreWebsocket& sock) const override;
};