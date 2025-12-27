#include <PartyDetailsNotification.h>
#include <ConnectionManager.h>

PartyDetailsNotification::PartyDetailsNotification(const std::string& partyId) :
	Notification(SpectreRpcType("PartyRpc.PartyDetailsV1Notification")) {
	payload = PartyDatabase::Get().GetPartyRes(partyId);
}

PartyDetailsNotification::PartyDetailsNotification(const PartyResponse& res) :
	Notification(SpectreRpcType("PartyRpc.PartyDetailsV1Notification")) {
	payload = res;
}

PartyDetailsNotification::PartyDetailsNotification(const Party& party) :
	Notification(SpectreRpcType("PartyRpc.PartyDetailsV1Notification")) {
	payload.mutable_party()->CopyFrom(party);
}

void PartyDetailsNotification::SendTo(SpectreWebsocket& sock) const {
	sock.SendNotification(PartyDatabase::SerializePartyToString(payload), GetNotificationType());
}

void PartyDetailsNotification::SendToAllInParty() const
{
	std::string partyStr = PartyDatabase::SerializePartyToString(payload);
	for (int i = 0; i < payload.party().partymembers_size(); i++)
	{
		ConnectionManager::Get().GetConnectionFromPlayerId(payload.party().partymembers(i).playerid())->SendNotification(partyStr, GetNotificationType());
	}
}