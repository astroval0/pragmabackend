#pragma once
#include <PacketProcessor.h>

class GetFriendsListProcessor : public WebsocketPacketProcessor {
public:
    GetFriendsListProcessor(SpectreRpcType rpcType);

    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};