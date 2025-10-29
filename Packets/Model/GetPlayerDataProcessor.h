#pragma once
#include <PacketProcessor.h>

class GetPlayerDataProcessor : public WebsocketPacketProcessor {
public:
    GetPlayerDataProcessor(SpectreRpcType rpcType);

    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};