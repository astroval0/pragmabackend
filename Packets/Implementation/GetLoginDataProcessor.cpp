#include <GetLoginDataProcessor.h>
#include <GameDataStore.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>
#include <PlayerDatabase.h>
#include <PlayerData.pb.h>
#include <GetPlayerDataProcessor.h>
#include <chrono>
#include <date/date.h>

namespace pbu = google::protobuf::util;

GetLoginDataProcessor::GetLoginDataProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void GetLoginDataProcessor::Process(SpectreWebsocketRequest& request, SpectreWebsocket& sock) {
	// todo add player data
	std::string loginDataRes = "{\"loginData\":{\"ext\":{\"inboxMessages\":[],\"playerData\":";
	std::unique_ptr<PlayerData> playerData = PlayerDatabase::Get().GetField<PlayerData>(FieldKey::PLAYER_DATA, sock.GetPlayerId());
	loginDataRes += GetPlayerDataProcessor::GetPlayerDataAsString(*playerData);
	auto now = std::chrono::system_clock::now();
	auto crewResetTime = now + std::chrono::days(7);
	loginDataRes += ",\"noCrew\":-1.0,\"nextCrewAutomationDate\":\"2025-03-04T09:00\",\"crewAutomationInProgress\":false,\"currentServiceTimestampMillis\":\"" 
		+ std::to_string(duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count())
		+ "\"},\"inventoryData\":{\"issuedLimitedGrantTrackingIds\":[],\"inventoryContent\":";
	loginDataRes += *GameDataStore::Get().InventoryStore_buf();
	loginDataRes += "}}}";
	sock.SendPacket(loginDataRes, request.GetRequestId(), request.GetResponseType());
}