#include <LoginToChatProcessor.h>
#include <VivoxTokenGenerator.h>
#include <spdlog/spdlog.h>

#include "SpectreRpcType.h"

//todo move to auth.json
const std::string VIVOX_DOMAIN = "mtu1xp.vivox.com";
const std::string VIVOX_ISSUER = "57732-spect-33067-udash";
const std::string VIVOX_KEY = "CGnn6pgN45CFQpNhl0gNTVP3McY59Sty";
const std::string VIVOX_SERVER = "https://mtu1xp.www.vivox.com/api2";

LoginToChatProcessor::LoginToChatProcessor(const SpectreRpcType &rpcType) : WebsocketPacketProcessor(rpcType) {}

void LoginToChatProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
    std::string playerId = sock.GetPlayerId();

    std::string loginToken = VivoxTokenGenerator::Generate(VIVOX_KEY, VIVOX_ISSUER, VIVOX_DOMAIN, playerId, "login");
    std::string joinToken = VivoxTokenGenerator::Generate(VIVOX_KEY, VIVOX_ISSUER, VIVOX_DOMAIN, playerId, "join", "global");

    const std::shared_ptr<json> response = packet.GetBaseJsonResponse();
    json& payload = (*response)["payload"];

    payload["success"] = true;
    payload["token"] = loginToken;

    payload["generalchatroom"] = "global";
    payload["generalchattoken"] = joinToken;

    payload["chatserver"] = VIVOX_SERVER;
    payload["chatdomain"] = VIVOX_DOMAIN;
    payload["chatissuer"] = VIVOX_ISSUER;

    spdlog::info("vivox login gen for player {}", playerId);
    sock.SendPacket(response);
}