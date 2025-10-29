#pragma once
#include <PacketProcessor.h>

class SaveOutfitLoadoutProcessor : public WebsocketPacketProcessor {
public:
    SaveOutfitLoadoutProcessor(SpectreRpcType rpcType);

    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};