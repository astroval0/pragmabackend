#include <GetPlayerDataProcessor.h>
#include <PlayerDatabase.h>
#include <PlayerData.pb.h>

static const std::string endOfConfigJson = "\"inputBindingsVersion\":3}";

GetPlayerDataProcessor::GetPlayerDataProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

std::string GetPlayerDataProcessor::GetPlayerDataAsString(const PlayerData& playerData) {
	std::string playerDataComponent;
	pbuf::util::JsonPrintOptions popts;
	popts.always_print_fields_with_no_presence = true;
	auto status = pbuf::util::MessageToJsonString(playerData, &playerDataComponent, popts);
	if (!status.ok()) {
		spdlog::error("Failed to serialize pbuf PlayerData message to string: {}", status.message());
		throw;
	}
	// mt wanted the PlayerConfig object to be a json STRING for some reason, probably to avoid
	// having to serialize a 100 field object which would be slow? either way, we do this... interesting thing to quote it the way the game expects
	// All of the field sizes up to this point are constant, so we can use a magic number to quote the start of the config field
	std::string finalPlayerDataComponent(playerDataComponent.begin(), playerDataComponent.begin() + 58);
	finalPlayerDataComponent += "\"{";
	int endIndex = 0;
	for (int i = 59; i < playerDataComponent.size(); i++) {
		char curChar = playerDataComponent[i];
		if (curChar == '}') {
			if (playerDataComponent.compare(i - endOfConfigJson.length() + 1, endOfConfigJson.length(), endOfConfigJson.c_str()) == 0) {
				finalPlayerDataComponent += "}\"";
				endIndex = i;
				break;
			}
		}
		else if (curChar == '\"') {
			finalPlayerDataComponent += "\\\"";
		}
		else {
			finalPlayerDataComponent += curChar;
		}
	}
	if (endIndex == 0) {
		spdlog::error("Failed to find end of PlayerConfig json object");
		throw;
	}
	finalPlayerDataComponent += playerDataComponent.c_str() + endIndex + 1;
	finalPlayerDataComponent += "\"serverData\":\"{}\",";
	return finalPlayerDataComponent;
}

void GetPlayerDataProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<PlayerData> data = PlayerDatabase::Get().GetField<PlayerData>(FieldKey::PLAYER_DATA, sock.GetPlayerId());
	std::string fullPayload = "{\"data\":";
	fullPayload += GetPlayerDataAsString(*data) + "}";
	sock.SendPacket(fullPayload, packet.GetRequestId(), packet.GetResponseType());
}