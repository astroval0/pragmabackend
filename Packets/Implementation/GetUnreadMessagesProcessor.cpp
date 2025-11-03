#include <GetUnreadMessagesProcessor.h>
#include <PlayerDatabase.h>
#include <ClientMessages.pb.h>

GetUnreadMessagesProcessor::GetUnreadMessagesProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void GetUnreadMessagesProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<ClientMessages> unreadMessages = PlayerDatabase::Get().GetField<ClientMessages>(FieldKey::PLAYER_UNDELIVERED_MESSAGES, sock.GetPlayerId());
	sock.SendPacket(*unreadMessages, packet.GetResponseType(), packet.GetRequestId());
}