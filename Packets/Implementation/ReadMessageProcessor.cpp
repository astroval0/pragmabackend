#include <ReadMessageProcessor.h>
#include <ClientMessages.pb.h>
#include <ReadMessage.pb.h>
#include <PlayerDatabase.h>

ReadMessageProcessor::ReadMessageProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void ReadMessageProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<ReadMessage> payload = packet.GetPayloadAsMessage<ReadMessage>();
	std::unique_ptr<ClientMessages> msgs = PlayerDatabase::Get().GetField<ClientMessages>(FieldKey::PLAYER_UNDELIVERED_MESSAGES, sock.GetPlayerId());
	for (int i = 0; i < msgs->messages_size(); i++) {
		if (msgs->messages(i).messageid() == payload->messageid()) {
			msgs->mutable_messages()->DeleteSubrange(i, i);
			break;
		}
	}
	PlayerDatabase::Get().SetField(FieldKey::PLAYER_UNDELIVERED_MESSAGES, msgs.get(), sock.GetPlayerId());
	sock.SendPacket(packet.GetBaseJsonResponse());
}