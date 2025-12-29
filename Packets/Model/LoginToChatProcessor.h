#pragma once
#include <PacketProcessor.h>

class LoginToChatProcessor final : public WebsocketPacketProcessor {
public:
    explicit LoginToChatProcessor(const SpectreRpcType &rpcType);
    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};