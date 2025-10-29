#pragma once
#include <PacketProcessor.h>

class UpdateItemsV0Processor : public WebsocketPacketProcessor {
public:
    UpdateItemsV0Processor(SpectreRpcType rpcType);

    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};