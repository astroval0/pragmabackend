#include <StaticResponseProcessorWS.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

void StaticResponseProcessorWS::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::shared_ptr<json> res = packet.GetBaseJsonResponse();
	(*res)["payload"] = *m_res;
	sock.SendPacket(res);
}