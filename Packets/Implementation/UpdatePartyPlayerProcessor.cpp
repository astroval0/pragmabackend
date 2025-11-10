#include <UpdatePartyPlayerProcessor.h>
#include <PartyDatabase.h>
#include <UpdatePartyPlayerRequest.pb.h>
#include <PartyMember.pb.h>

UpdatePartyPlayerProcessor::UpdatePartyPlayerProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void UpdatePartyPlayerProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<UpdatePartyPlayerRequest> req = packet.GetPayloadAsMessage<UpdatePartyPlayerRequest>();
	PartyResponse res = PartyDatabase::Get().GetPartyRes(req->partyid());
	Party* party = res.mutable_party();
	PartyMemberExtraInfo* memberExt = nullptr;
	for (int i = 0; i < party->partymembers_size(); i++) {
		if (party->partymembers(i).playerid() == sock.GetPlayerId()) {
			memberExt = party->mutable_partymembers(i)->mutable_ext();
		}
	}
	if (memberExt == nullptr) {
		spdlog::warn("could not find player(id: {}) in party(id: {}) that was being updated by UpdatePartyPlayerProcessor");
		return;
	}
	memberExt->set_version(req->requestext().version());
	memberExt->set_region(req->requestext().region());
	memberExt->set_preferredteam(req->requestext().preferredteam());
	// ignoring refreshData, preferredRegions, regionPings, sharedClientData for the moment as they don't seem to be important or used for customgame
	PartyDatabase::Get().SaveParty(res.party());
	sock.SendPacket(PartyDatabase::SerializePartyToString(res), packet.GetRequestId(), packet.GetResponseType());
}