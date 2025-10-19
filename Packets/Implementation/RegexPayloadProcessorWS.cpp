#include <RegexPayloadProcessorWS.h>

void RegexPayloadProcessorWS::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
    std::shared_ptr<json> res = packet.GetBaseJsonResponse();
    std::string payloadStr = packet.GetPayload()->dump();
    for (const auto& [regex, payload] : m_resMap) {
        if (std::regex_search(payloadStr, regex.m_rx)) {
            (*res)["payload"] = *payload;
            sock.SendPacket(res);
            return;
        }
    }
}