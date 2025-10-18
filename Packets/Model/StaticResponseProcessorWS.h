#pragma once
#include <PacketProcessor.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

class StaticResponseProcessorWS : public WebsocketPacketProcessor {
private:
    std::shared_ptr<json> m_res;
public:
    StaticResponseProcessorWS(SpectreRpcType rpcType, std::shared_ptr<json> res)
        : WebsocketPacketProcessor(rpcType), m_res(res) {
    };

    void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};