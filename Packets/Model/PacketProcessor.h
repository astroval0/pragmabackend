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
#include "Site.h"

namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;
using tls_stream = boost::asio::ssl::stream<tcp::socket>;

class HTTPPacketProcessor {
private:
    std::string m_route;
    inline static std::unordered_map<std::string, HTTPPacketProcessor*> HTTP_ROUTES = {};
    static std::string Key(const std::string& route, Site site) {
        return route + "|" + SiteKey(site);
    }
public:
    HTTPPacketProcessor(std::string route, Site site) : m_route(std::move(route)) {
        HTTP_ROUTES[Key(m_route, site)] = this;
    };
    virtual void Process(http::request<http::string_body> const& req, tls_stream& sock) = 0;
    virtual ~HTTPPacketProcessor() = default;
    std::string& GetRoute() const {
        return const_cast<std::string&>(m_route);
    }
    static HTTPPacketProcessor* GetProcessorForRoute(const std::string& route, Site site) {
        auto it = HTTP_ROUTES.find(Key(route, site));
        return it == HTTP_ROUTES.end() ? nullptr : it->second;
    }
};

class WebsocketPacketProcessor {
private:
    SpectreRpcType m_rpcType;
    inline static std::unordered_map<std::string, WebsocketPacketProcessor*> WEBSOCKET_ROUTES = {};
    static std::string Key(const SpectreRpcType& t, Site site) {
        return t.GetName() + std::string("|") + SiteKey(site);
    }
public:
    WebsocketPacketProcessor(const SpectreRpcType& rpcType, Site site) : m_rpcType(rpcType) {
        WEBSOCKET_ROUTES[Key(m_rpcType, site)] = this;
    }
    virtual void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) = 0;
    virtual ~WebsocketPacketProcessor() = default;
    const SpectreRpcType& GetType() {
        return m_rpcType;
    }
    static WebsocketPacketProcessor* GetProcessorForRpc(const SpectreRpcType& rpcType, Site site) {
        auto it = WEBSOCKET_ROUTES.find(Key(rpcType, site));
        return it == WEBSOCKET_ROUTES.end() ? nullptr : it->second;
    }
};