#include <StaticResponseProcessorWS.h>
#include <nlohmann/json.hpp>

StaticResponseProcessorWS::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override {
	nlohmann::json& payload = GetBaseJsonResponse();
	payload = m_res;
	sock.SendPacket(payload);
}