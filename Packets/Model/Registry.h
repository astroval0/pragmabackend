#pragma once

#include <map>
#include <string>
#include <PacketProcessor.h>

class Registry {
public:
	inline static std::map<std::string, HTTPPacketProcessor*> HTTP_ROUTES = {};
	inline static std::map<std::string, WebsocketPacketProcessor*> WEBSOCKET_ROUTES = {};
};