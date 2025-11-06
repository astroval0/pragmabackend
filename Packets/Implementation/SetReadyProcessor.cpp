#include <SetReadyProcessor.h>
#include <PartyDatabase.h>
#include <PlayerDatabase.h>
#include <SetReadyMessage.pb.h>

SetReadyProcessor::SetReadyProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void SetReadyProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<SetReadyMessage> readymsg = packet.GetPayloadAsMessage<SetReadyMessage>();
}