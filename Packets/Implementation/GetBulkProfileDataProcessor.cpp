#include <GetBulkProfileDataProcessor.h>
#include <BulkProfileDataMessage.pb.h>
#include <ProfileData.pb.h>
#include <PlayerDatabase.h>

GetBulkProfileDataProcessor::GetBulkProfileDataProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void GetBulkProfileDataProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<GetBulkProfileDataMessage> req = packet.GetPayloadAsMessage<GetBulkProfileDataMessage>();
	BulkProfileDataResponse res;
	for (int i = 0; i < req->playerids_size(); i++) {
		res.add_bulkprofiledata()->CopyFrom(
			*PlayerDatabase::Get().GetField<ProfileData>(FieldKey::PROFILE_DATA, req->playerids(i))
		);
	}
	sock.SendPacket(res, packet.GetResponseType(), packet.GetRequestId());
}