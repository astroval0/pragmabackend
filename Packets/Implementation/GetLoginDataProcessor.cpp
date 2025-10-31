#include <GetLoginDataProcessor.h>
#include <GameDataStore.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>
#include <PlayerDatabase.h>
#include <PlayerData.pb.h>
#include <GetPlayerDataProcessor.h>

namespace pbu = google::protobuf::util;

GetLoginDataProcessor::GetLoginDataProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void GetLoginDataProcessor::Process(SpectreWebsocketRequest& request, SpectreWebsocket& sock) {
	// todo add player data
	std::string loginDataRes = "{\"loginData\":{\"ext\":{\"inboxMessages\":[],\"playerData\":";
	std::unique_ptr<PlayerData> playerData = PlayerDatabase::Get().GetField<PlayerData>(FieldKey::PLAYER_DATA, sock.GetPlayerId());
	loginDataRes += GetPlayerDataProcessor::GetPlayerDataAsString(*playerData);
	loginDataRes += ",\"noCrew\":-1.0,\"nextCrewAutomationDate\":\"hits\",\"crewAutomationInProgress\":false,\"currentServiceTimestampMillis\":\"\"},\"inventoryData\":{\"issuedLimitedGrantTrackingIds\":[],\"inventoryContent\":";
	loginDataRes += *GameDataStore::Get().InventoryStore_buf();
	loginDataRes += "}}}";
	sock.SendPacket(loginDataRes, request.GetRequestId(), request.GetResponseType());
}