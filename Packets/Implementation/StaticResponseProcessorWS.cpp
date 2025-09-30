#include <StaticResponseProcessorWS.h>
#include <nlohmann/json.hpp>

void StaticResponseProcessorWS::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	nlohmann::json payload = packet.GetBaseJsonResponse();
	payload = m_res;
	sock.SendPacket(payload);
}