#pragma once
#include <PacketProcessor.h>

class SetPresenceProcessor : public WebsocketPacketProcessor
{
public:
    SetPresenceProcessor(SpectreRpcType rpcType);

    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};