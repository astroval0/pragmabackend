#include <StaticResponseProcessorWS.h>
#include <nlohmann/json.hpp>

void StaticResponseProcessorWS::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	nlohmann::json res = packet.GetBaseJsonResponse();
	res["response"]["payload"] = m_res;
	sock.SendPacket(res);
}