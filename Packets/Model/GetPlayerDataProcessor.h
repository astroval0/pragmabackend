#pragma once
#include <PacketProcessor.h>
#include <PlayerData.pb.h>

class GetPlayerDataProcessor : public WebsocketPacketProcessor {
public:
    GetPlayerDataProcessor(SpectreRpcType rpcType);

    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
    static std::string GetPlayerDataAsString(const PlayerData& playerData);
};