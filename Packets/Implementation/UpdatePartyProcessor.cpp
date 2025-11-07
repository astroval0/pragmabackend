#include <UpdatePartyProcessor.h>
#include <UpdatePartyRequest.pb.h>
#include <PartyDatabase.h>

UpdatePartyProcessor::UpdatePartyProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void UpdatePartyProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<UpdatePartyRequest> req = packet.GetPayloadAsMessage<UpdatePartyRequest>();
	PartyResponse res = PartyDatabase::Get().GetPartyRes(req->partyid());
	res.mutable_party()->mutable_extbroadcastparty()->CopyFrom(req->requestext());
	PartyDatabase::Get().SaveParty(res.party());
	sock.SendPacket(PartyDatabase::SerializePartyToString(res), packet.GetRequestId(), packet.GetResponseType());
}