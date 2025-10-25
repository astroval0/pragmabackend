#pragma once
#include <PacketProcessor.h>

namespace fs = std::filesystem;

class UpdateItemsV0Processor : public WebsocketPacketProcessor {
public:
    UpdateItemsV0Processor(SpectreRpcType rpcType);

    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};