#include <UpdatePartyProcessor.h>
#include <UpdatePartyRequest.pb.h>
#include <PartyDatabase.h>
#include <string>

UpdatePartyProcessor::UpdatePartyProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void UpdatePartyProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<UpdatePartyRequest> req = packet.GetPayloadAsMessage<UpdatePartyRequest>();
	PartyResponse res = PartyDatabase::Get().GetPartyRes(req->partyid());
	Party* party = res.mutable_party();
	BroadcastPartyExtraInfo* broadcastExtra = party->mutable_extbroadcastparty();
	broadcastExtra->set_pool(req->requestext().pool());
	broadcastExtra->set_lobbymode(req->requestext().lobbymode());
	if (req->requestext().lobbymode() == "custom") {
		broadcastExtra->mutable_custom()->CopyFrom(req->requestext().custom());
	}
	// Probably need to handle removePlayers at some point but I don't have a request example atm so leaving it for now
	broadcastExtra->set_version(req->requestext().version());
	broadcastExtra->set_region(req->requestext().region());
	broadcastExtra->set_tag(req->requestext().tag());
	broadcastExtra->set_profile(req->requestext().profile());
	broadcastExtra->set_useteammmr(req->requestext().useteammmr());
	broadcastExtra->set_hasacceptableregion(req->requestext().acceptableregions_size() > 0);
	for (const auto& entry : req->requestext().standard()) {
		(*broadcastExtra->mutable_standard())[entry.first] = entry.second;
	}
	party->set_version(std::to_string(stoi(party->version()) + 1));
	PartyDatabase::Get().SaveParty(res.party());
	sock.SendPacket(PartyDatabase::SerializePartyToString(res), packet.GetRequestId(), packet.GetResponseType());
}