#pragma once
#include <PacketProcessor.h>

class ReadMessageProcessor : public WebsocketPacketProcessor {
public:
	ReadMessageProcessor(SpectreRpcType rpcType);
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};