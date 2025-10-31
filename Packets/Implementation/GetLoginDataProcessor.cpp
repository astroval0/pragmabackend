#include <GetLoginDataProcessor.h>
#include <GameDataStore.h>

GetLoginDataProcessor::GetLoginDataProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void GetLoginDataProcessor::Process(SpectreWebsocketRequest& request, SpectreWebsocket& sock) {
	// todo add player data
	std::string copy = *GameDataStore::Get().InventoryStore_buf();
	sock.SendPacket(copy, request.GetRequestId(), request.GetResponseType());
}