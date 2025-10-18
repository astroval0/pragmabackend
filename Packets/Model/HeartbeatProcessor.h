#include <PacketProcessor.h>

class HeartbeatProcessor : public WebsocketPacketProcessor {
public:
	HeartbeatProcessor(SpectreRpcType rpcType, Site site) : WebsocketPacketProcessor(rpcType, site) {};
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override {
		packet.SendEmptyResponse();
	}
};