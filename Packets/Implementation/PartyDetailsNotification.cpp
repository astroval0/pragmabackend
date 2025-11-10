#include <PartyDetailsNotification.h>

PartyDetailsNotification::PartyDetailsNotification(const std::string& partyId, SpectreRpcType notificationType) :
	Notification(notificationType) {
	payload = PartyDatabase::Get().GetPartyRes(partyId);
}

PartyDetailsNotification::PartyDetailsNotification(const PartyResponse& res, SpectreRpcType notificationType) :
	Notification(notificationType) {
	payload = res;
}

PartyDetailsNotification::PartyDetailsNotification(const Party& party, SpectreRpcType notificationType) :
	Notification(notificationType) {
	payload.mutable_party()->CopyFrom(party);
}

void PartyDetailsNotification::SendTo(SpectreWebsocket& sock) const {
	sock.SendNotification(PartyDatabase::SerializePartyToString(payload), GetNotificationType());
}