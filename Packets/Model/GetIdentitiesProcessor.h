#include <PacketProcessor.h>
#include <SteamValidator.h>

class GetIdentitiesProcessor : public WebsocketPacketProcessor
{
public:
    GetIdentitiesProcessor(SpectreRpcType rpcType);

    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
    static SteamValidator steamValidator;
};