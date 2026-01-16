#pragma once
#include <SpectreWebsocket.h>
#include <unordered_map>

class ConnectionManager
{
private:
    std::unordered_map<std::string, SpectreWebsocket*> m_wsByPlayerId;
    static ConnectionManager m_inst;
public:
    ConnectionManager();
    static ConnectionManager& Get();
    [[nodiscard]] SpectreWebsocket* GetConnectionFromPlayerId(const std::string& playerId) const;
    [[nodiscard]] SpectreWebsocket* TryGetConnectionFromPlayerId(const std::string& playerId) const;
    void AddNewConnection(const std::string& playerId, SpectreWebsocket* connection);
};