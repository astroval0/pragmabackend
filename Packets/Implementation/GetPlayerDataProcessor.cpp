#include <GetPlayerDataProcessor.h>
#include <PlayerDatabase.h>
#include <PlayerData.pb.h>

GetPlayerDataProcessor::GetPlayerDataProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void GetPlayerDataProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<PlayerData> data = PlayerDatabase::Get().GetField<PlayerData>(FieldKey::PLAYER_DATA, sock.GetPlayerId());
}