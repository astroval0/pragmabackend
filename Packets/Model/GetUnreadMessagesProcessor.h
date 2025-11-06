#pragma once
#include <PacketProcessor.h>

class GetUnreadMessagesProcessor : public WebsocketPacketProcessor {
public:
	GetUnreadMessagesProcessor(SpectreRpcType rpcType);
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};