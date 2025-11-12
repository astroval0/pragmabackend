#pragma once
#include <PacketProcessor.h>

class SetReadyProcessor : public WebsocketPacketProcessor {
public:
	SetReadyProcessor(SpectreRpcType rpcType);
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};