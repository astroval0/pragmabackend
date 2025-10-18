#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <SpectreWebsocketRequest.h>
#include <SpectreRpcType.h>
#include <string>

class HTTPPacketProcessor {
private:
    std::string m_route;
    inline static std::unordered_map<std::string, HTTPPacketProcessor*> HTTP_ROUTES = {};
public:
    HTTPPacketProcessor(std::string route) : m_route(std::move(route)) {
        HTTP_ROUTES[m_route] = this;
    };
    virtual void Process(http::request<http::string_body> const& req, tcp::socket& sock) = 0;
    virtual ~HTTPPacketProcessor() = default;
    std::string& GetRoute() const {
        return const_cast<std::string&>(m_route);
    }
    static HTTPPacketProcessor* GetProcessorForRoute(const std::string& route) {
        auto it = HTTP_ROUTES.find(route);
        return it == HTTP_ROUTES.end() ? nullptr : it->second;
    }
};

class WebsocketPacketProcessor {
private:
    SpectreRpcType m_rpcType;
    inline static std::unordered_map<SpectreRpcType, WebsocketPacketProcessor*> WEBSOCKET_ROUTES = {};
public:
    WebsocketPacketProcessor(const SpectreRpcType& rpcType) : m_rpcType(rpcType) {
        WEBSOCKET_ROUTES[rpcType] = this;
    }
    virtual void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) = 0;
    virtual ~WebsocketPacketProcessor() = default;
    const SpectreRpcType& GetType() {
        return m_rpcType;
    }
    static WebsocketPacketProcessor* GetProcessorForRpc(const SpectreRpcType& rpcType) {
        auto it = WEBSOCKET_ROUTES.find(rpcType);
        return it == WEBSOCKET_ROUTES.end() ? nullptr : it->second;
    }
};