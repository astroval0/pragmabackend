#pragma once
#include <PacketProcessor.h>

class GetLoginDataProcessor : public WebsocketPacketProcessor {
public:
	GetLoginDataProcessor(SpectreRpcType rpcType);

	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};