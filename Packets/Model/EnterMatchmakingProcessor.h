#pragma once
#include <PacketProcessor.h>

class EnterMatchmakingProcessor : public WebsocketPacketProcessor {
public:
	EnterMatchmakingProcessor(SpectreRpcType rpcType);
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};