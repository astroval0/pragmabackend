#include <StaticResponseProcessorWS.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

void StaticResponseProcessorWS::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	nlohmann::json res = packet.GetBaseJsonResponse();
	json payloadcopy = m_res;
	spdlog::info(m_res.dump()); // crashes right here, have already verified that m_res is good by dumping it in the constructor. wtf.
	res["response"]["payload"] = payloadcopy;
	sock.SendPacket(res);
}