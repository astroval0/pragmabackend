#pragma once
#include <PacketProcessor.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
#include "Regex.h"

class RegexPayloadProcessorWS : public WebsocketPacketProcessor {
private:
	std::unordered_map<regex, std::shared_ptr<json>> m_resMap;
public:
	RegexPayloadProcessorWS(SpectreRpcType rpcType, std::unordered_map<regex, std::shared_ptr<json>> resMap)
		: WebsocketPacketProcessor(rpcType), m_resMap(resMap) {
	};

	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};