#include <PacketProcessor.h>

class HeartbeatProcessor : public WebsocketPacketProcessor {
public:
	HeartbeatProcessor(SpectreRpcType rpcType) : WebsocketPacketProcessor(rpcType) {};
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override {
		packet.SendEmptyResponse();
	}
};