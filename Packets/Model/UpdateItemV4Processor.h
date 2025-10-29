#pragma once
#include <PacketProcessor.h>

class UpdateItemV4Processor : public WebsocketPacketProcessor {
public:
    UpdateItemV4Processor(SpectreRpcType rpcType);

    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};