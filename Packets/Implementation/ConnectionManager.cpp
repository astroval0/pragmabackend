#include <ConnectionManager.h>
#include <spdlog/spdlog.h>

ConnectionManager ConnectionManager::m_inst;

ConnectionManager& ConnectionManager::Get()
{
    return m_inst;
}

ConnectionManager::ConnectionManager()
{

}

SpectreWebsocket* ConnectionManager::GetConnectionFromPlayerId(const std::string& playerId) const
{
    std::unordered_map<std::string, SpectreWebsocket*>::const_iterator it = m_wsByPlayerId.find(playerId);
    if (it == m_wsByPlayerId.end())
    {
        spdlog::error("Failed to get websocket for player with id: {}", playerId);
        throw std::runtime_error("Failed to find websocket for player");
    }
    return it->second;
}

SpectreWebsocket* ConnectionManager::TryGetConnectionFromPlayerId(const std::string& playerId) const
{
    std::unordered_map<std::string, SpectreWebsocket*>::const_iterator it = m_wsByPlayerId.find(playerId);
    if (it == m_wsByPlayerId.end())
    {
        return nullptr;
    }
    return it->second;
}

void ConnectionManager::AddNewConnection(const std::string& playerId, SpectreWebsocket* connection)
{
    m_wsByPlayerId.insert(std::make_pair(playerId, connection));
}