#include <GetFriendsListProcessor.h>
#include <PlayerDatabase.h>

#include <FriendsList.pb.h>
#include <PresenceStatus.pb.h>
#include <FriendsListResponse.pb.h>

GetFriendsListProcessor::GetFriendsListProcessor(SpectreRpcType rpcType) : WebsocketPacketProcessor(rpcType)
{

}

void GetFriendsListProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock)
{
    std::unique_ptr<PresenceStatus> playerPresence = PlayerDatabase::Get().GetField<PresenceStatus>(FieldKey::PLAYER_PRESENCE_STATUS, sock.GetPlayerId());
    playerPresence->set_presence(PresenceState::Online);
    const std::unique_ptr<FriendsList> friends = PlayerDatabase::Get().GetField<FriendsList>(FieldKey::PLAYER_FRIENDS_LIST, sock.GetPlayerId());
    FriendsListResponse res;
    res.mutable_friendlist()->set_acceptnewfriendinvites(friends->acceptnewfriendinvites());
    res.mutable_friendlist()->mutable_friends()->CopyFrom(friends->friends());
    res.mutable_friendlist()->mutable_receivedinvites()->CopyFrom(friends->receivedinvites());
    res.mutable_friendlist()->mutable_sentinvites()->CopyFrom(friends->sentinvites());
    res.mutable_friendlist()->mutable_blocked()->CopyFrom(friends->blocked());
    res.mutable_friendlist()->set_version("0");
    sock.SendPacket(res, packet.GetResponseType(), packet.GetRequestId());
}