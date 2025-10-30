#include <GetPlayerDataProcessor.h>
#include <PlayerDatabase.h>
#include <PlayerData.pb.h>

static const std::string endOfConfigJson = "\"inputBindingsVersion\":3}";

GetPlayerDataProcessor::GetPlayerDataProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void GetPlayerDataProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<PlayerData> data = PlayerDatabase::Get().GetField<PlayerData>(FieldKey::PLAYER_DATA, sock.GetPlayerId());
	std::string fullPayload = "{\"data\":";
	std::string resComponent;
	if (!pbuf::util::MessageToJsonString(*data, &resComponent).ok()) {
		spdlog::error("Failed to serialize pbuf PlayerData message to string in GetPlayerDataProcessor");
		throw;
	}
	fullPayload += resComponent + "}";
	// mt wanted the PlayerConfig object to be a json STRING for some reason, probably to avoid
	// having to serialize a 100 field object which would be slow? either way, we do this... interesting thing to quote it the way the game expects
	// All of the field sizes up to this point are constant, so we can use a magic number to quote the start of the config field
	std::string finalRes(fullPayload.begin(), fullPayload.begin() + 66);
	finalRes += "\"{";
	int endIndex = 0;
	for (int i = 67; i < fullPayload.size(); i++) {
		char curChar = fullPayload[i];
		if (curChar == '}') {
			if (fullPayload.compare(i - endOfConfigJson.length() + 1, endOfConfigJson.length(), endOfConfigJson.c_str()) == 0) {
				finalRes += "}\"";
				endIndex = i;
				break;
			}
		}
		else if (curChar == '\"') {
			finalRes += "\\\"";
		}
		else {
			finalRes += curChar;
		}
	}
	if (endIndex == 0) {
		spdlog::error("Failed to find end of PlayerConfig json object");
		throw;
	}
	finalRes += fullPayload.c_str() + endIndex + 1;
	sock.SendPacket(finalRes, packet.GetRequestId(), packet.GetResponseType());
}