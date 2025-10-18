#include <RegexPayloadProcessorWS.h>
#include <nlohmann/json.hpp>

void RegexPayloadProcessorWS::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::shared_ptr<nlohmann::json> res = packet.GetBaseJsonResponse();
	std::string payloadStr = packet.GetPayload()->dump();
	for (const auto& [regex, payload] : m_resMap) {
		if (std::regex_match(payloadStr, regex.m_rx)) {
			(*res)["response"]["payload"] = *payload;
			sock.SendPacket(res);
			return;
		}
	}
}