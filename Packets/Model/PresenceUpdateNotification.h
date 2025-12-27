#pragma once
#include <Notification.h>
#include <string>
#include <PresenceUpdateNotificationContent.pb.h>

class PresenceUpdateNotification : public Notification
{
private:
    PresenceUpdateNotificationContent m_notificationContent;
    std::string m_playerId;
public:
    PresenceUpdateNotification(SpectreRpcType, const std::string& playerId);

    void SendToAllFriends() const;
    void SendTo(SpectreWebsocket& sock) const override;
};