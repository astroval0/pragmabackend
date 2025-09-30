#pragma once
#include <PacketProcessor.h>
#include <nlohmann/json.hpp>

class StaticResponseProcessorWS : public WebsocketPacketProcessor {
private:
	nlohmann::json& m_res;
public:
	StaticResponseProcessorWS(SpectreRpcType rpcType, nlohmann::json& res)
		: WebsocketPacketProcessor(rpcType), m_res(res) {};

	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};