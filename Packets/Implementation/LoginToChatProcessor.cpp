#include <LoginToChatProcessor.h>
#include <VivoxTokenGenerator.h>
#include <spdlog/spdlog.h>
#include "SpectreRpcType.h"
#include <nlohmann/json.hpp>
#include <fstream>

struct VivoxConfig {
    std::string domain;
    std::string issuer;
    std::string key;
    std::string server;
};

static VivoxConfig LoadVivoxCfg() {
    VivoxConfig cfg;

    try {
        std::ifstream f("auth.json");

        nlohmann::json j;
        f >> j;
        const auto&  v = j["vivox"];

        cfg.domain = v.value("domain", "");
        cfg.issuer = v.value("issuer", "");
        cfg.key = v.value("key", "");
        cfg.server = v.value("server", "");

    } catch ([[maybe_unused]] const std::exception& e) {
        spdlog::warn("failed to parse auth.json, probably missing cfg");
    }

    return cfg;
}

LoginToChatProcessor::LoginToChatProcessor(const SpectreRpcType &rpcType) : WebsocketPacketProcessor(rpcType) {}

void LoginToChatProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
    const auto cfg = LoadVivoxCfg();
    const std::string playerId = sock.GetPlayerId();

    const std::shared_ptr<json> response = packet.GetBaseJsonResponse();
    json& payload = (*response)["payload"];

    const std::string loginToken = VivoxTokenGenerator::Generate(
        cfg.key,
        cfg.issuer,
        cfg.domain,
        playerId,
        "login"
        );

    const std::string joinToken = VivoxTokenGenerator::Generate(
        cfg.key,
        cfg.issuer,
        cfg.domain,
        playerId,
        "join",
        "global"
        );

    payload["success"] = true;
    payload["token"] = loginToken;

    payload["generalchatroom"] = "global";
    payload["generalchattoken"] = joinToken;

    payload["chatserver"] = cfg.server;
    payload["chatdomain"] = cfg.domain;
    payload["chatissuer"] = cfg.issuer;

    // spdlog::info("vivox login gen for player {}", playerId);
    sock.SendPacket(response);
}