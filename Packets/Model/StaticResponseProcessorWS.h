#pragma once
#include <PacketProcessor.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

class StaticResponseProcessorWS : public WebsocketPacketProcessor {
private:
	nlohmann::json& m_res;
public:
	StaticResponseProcessorWS(SpectreRpcType rpcType, nlohmann::json res)
		: WebsocketPacketProcessor(rpcType), m_res(res) {
		spdlog::info(res.dump());
		spdlog::info(m_res.dump());
	};

	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};