#include <SetPresenceProcessor.h>
#include <SetPresenceRequest.pb.h>
#include <PresenceStatus.pb.h>
#include "PlayerDatabase.h"
#include <PresenceUpdateNotification.h>

SetPresenceProcessor::SetPresenceProcessor(SpectreRpcType rpcType) : WebsocketPacketProcessor(rpcType)
{

}

void SetPresenceProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock)
{
    std::unique_ptr<SetPresenceRequest> req = packet.GetPayloadAsMessage<SetPresenceRequest>();
    PresenceStatus newPresence;
    newPresence.set_presence(req->basicpresence());
    PlayerDatabase::Get().SetField(FieldKey::PLAYER_PRESENCE_STATUS, &newPresence, sock.GetPlayerId());
    PresenceUpdateNotification notif(SpectreRpcType("FriendRpc.PresenceUpdateV1Notification"), sock.GetPlayerId());
    notif.SendToAllFriends();
    sock.SendPacket("{\"response\":\"Ok\"", packet.GetRequestId(), packet.GetResponseType());
}
