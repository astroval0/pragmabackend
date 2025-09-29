#pragma once

#include <unordered_map>
#include <string>
#include <PacketProcessor.h>
#include <functional>
#include <SpectreRpcType.h>

class Registry {
public:
	inline static std::unordered_map<std::string, HTTPPacketProcessor*> HTTP_ROUTES = {};
	inline static std::unordered_map<SpectreRpcType, WebsocketPacketProcessor*> WEBSOCKET_ROUTES = {};
};