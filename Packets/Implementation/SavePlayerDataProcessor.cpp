#include <SavePlayerDataProcessor.h>
#include <google/protobuf/util/json_util.h>
#include <PlayerDatabase.h>
#include <PlayerData.pb.h>

static const std::string endOfConfigJson = "\"inputBindingsVersion\": 3";

SavePlayerDataProcessor::SavePlayerDataProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void SavePlayerDataProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	const char* strbegin = reinterpret_cast<const char*>(packet.GetRawBuffer()->cdata().data());
	std::string reqFormatted = "{\"playerId\": \"" + sock.GetPlayerId() + "\",\"data\":";
	int index = 0;
	char curChar;
	for (; index < packet.GetRawBuffer()->size(); index++) {
		curChar = strbegin[index];
		if (curChar == '\"' && strbegin[index + 1] == '{') {
			index++; // skip the " character at the start of data str
			break;
		}
	}
	for (; index < packet.GetRawBuffer()->size(); index++) {
		curChar = strbegin[index];
		if (curChar == '\\') {
			if (strbegin[index + 1] == '\"') {
				reqFormatted += '\"';
			}
			index++;
		}
		else if (curChar == '}') {
			if (reqFormatted.compare(reqFormatted.size() - endOfConfigJson.size(), endOfConfigJson.size(), endOfConfigJson.c_str()) == 0) {
				reqFormatted += curChar;
				index += 2;
				break;
			}
			else {
				reqFormatted += curChar;
			}
		}
		else {
			reqFormatted += curChar;
		}
	}
	for (; index < packet.GetRawBuffer()->size() - 1; index++) {
		curChar = strbegin[index];
		reqFormatted += curChar;
	}
	PlayerData playerData;
	auto status = pbuf::util::JsonStringToMessage(reqFormatted, &playerData);
	if (!status.ok()) {
		spdlog::error("failed to read PlayerConfigData in SavePlayerDataProcessor: {}", status.message());
		throw;
	}
	// mt does this thing where they leave all the other fields blank if they just want to update the data str
	if (playerData.attackeroutfitloadoutid() == "") {
		// Only copy the extra data field
		std::unique_ptr<PlayerData> existingData = PlayerDatabase::Get().GetField<PlayerData>(FieldKey::PLAYER_DATA, sock.GetPlayerId());
		existingData->mutable_data()->CopyFrom(playerData.data());
		PlayerDatabase::Get().SetField(FieldKey::PLAYER_DATA, existingData.get(), sock.GetPlayerId());
	}
	else {
		spdlog::warn("attacker outfit loadout id set in SetPlayerData request from {}, this is not seen behavior, likely to cause bugs", sock.GetPlayerId());
		PlayerDatabase::Get().SetField(FieldKey::PLAYER_DATA, &playerData, sock.GetPlayerId());
	}
	std::shared_ptr<json> res = packet.GetBaseJsonResponse();
	(*res)["payload"]["success"] = true;
	sock.SendPacket(res);
}