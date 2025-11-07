#pragma once
#include <PacketProcessor.h>

class UpdatePartyProcessor : public WebsocketPacketProcessor {
public:
	UpdatePartyProcessor(SpectreRpcType rpcType);
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};