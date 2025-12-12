#include <PresenceUpdateNotification.h>
#include <PlayerDatabase.h>
#include <PresenceUpdateNotificationContent.pb.h>
#include <FriendsList.pb.h>
#include <ConnectionManager.h>

#include "PresenceStatus.pb.h"

PresenceUpdateNotification::PresenceUpdateNotification(SpectreRpcType rpcType, const std::string& playerId) :
    Notification(rpcType), m_playerId(playerId)
{
    m_notificationContent.mutable_newpresence()->set_playerid(playerId);
    m_notificationContent.mutable_newpresence()->set_gameshardid("00000000-0000-0000-0000-000000000000");
    m_notificationContent.mutable_newpresence()->set_gametitleid("00000000-0000-0000-0000-000000000001");
    m_notificationContent.mutable_newpresence()->set_version("1");
    std::unique_ptr<PresenceStatus> presence = PlayerDatabase::Get().GetField<PresenceStatus>(FieldKey::PLAYER_PRESENCE_STATUS, playerId);
    m_notificationContent.mutable_newpresence()->set_basicpresence(presence->presence());
}

void PresenceUpdateNotification::SendToAllFriends() const
{
    std::unique_ptr<FriendsList> flist = PlayerDatabase::Get().GetField<FriendsList>(FieldKey::PLAYER_FRIENDS_LIST, m_playerId);
    for (int i = 0; i < flist->friends_size(); i++)
    {
        ConnectionManager::Get().GetConnectionFromPlayerId(flist->friends(i))->SendNotification(m_notificationContent, GetNotificationType());
    }
}

void PresenceUpdateNotification::SendTo(SpectreWebsocket& sock) const
{
    sock.SendNotification(m_notificationContent, GetNotificationType());
}