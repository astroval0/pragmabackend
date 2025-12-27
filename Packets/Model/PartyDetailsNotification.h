#pragma once
#include <Notification.h>
#include <PartyDatabase.h>

class PartyDetailsNotification : public Notification {
private:
	PartyResponse payload;
public:
	PartyDetailsNotification(const std::string& partyId);
	PartyDetailsNotification(const PartyResponse& partyRes);
	PartyDetailsNotification(const Party& party);

	void SendTo(SpectreWebsocket& sock) const override;
	void SendToAllInParty() const;
};