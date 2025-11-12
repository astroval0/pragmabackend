#pragma once
#include <PacketProcessor.h>

class UpdatePartyPlayerProcessor : public WebsocketPacketProcessor {
public:
	UpdatePartyPlayerProcessor(SpectreRpcType rpcType);
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};