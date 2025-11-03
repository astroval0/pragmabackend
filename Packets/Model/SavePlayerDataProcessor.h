#pragma once
#include <PacketProcessor.h>

class SavePlayerDataProcessor : public WebsocketPacketProcessor {
public:
	SavePlayerDataProcessor(SpectreRpcType rpcType);

	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};