#pragma once
#include <PacketProcessor.h>

class GetBulkProfileDataProcessor : public WebsocketPacketProcessor {
public:
	GetBulkProfileDataProcessor(SpectreRpcType rpcType);

	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};