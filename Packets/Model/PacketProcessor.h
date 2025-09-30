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

namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;
using tls_stream = boost::asio::ssl::stream<tcp::socket>;

class HTTPPacketProcessor {
private:
	std::string& m_route;
	inline static std::unordered_map<std::string, HTTPPacketProcessor*> HTTP_ROUTES = {};
public:
	HTTPPacketProcessor(std::string route) : m_route(route) {
		HTTP_ROUTES[route] = this;
	};
	virtual void Process(http::request<http::string_body> const& req, tls_stream& sock) = 0;
	virtual ~HTTPPacketProcessor() = default;
	std::string& GetRoute() const {
		return m_route;
	}
	static HTTPPacketProcessor* GetProcessorForRoute(std::string& route) {
		return HTTP_ROUTES[route];
	}
};

class WebsocketPacketProcessor {
private:
	SpectreRpcType m_rpcType;
	inline static std::unordered_map<SpectreRpcType, WebsocketPacketProcessor*> WEBSOCKET_ROUTES = {};
public:
	WebsocketPacketProcessor(const SpectreRpcType& rpcType) : m_rpcType(rpcType) {
		WEBSOCKET_ROUTES[m_rpcType] = this;
	}
	virtual void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) = 0;
	virtual ~WebsocketPacketProcessor() = default;
	const SpectreRpcType& GetType() {
		return m_rpcType;
	}
	static WebsocketPacketProcessor* GetProcessorForRpc(const SpectreRpcType& rpcType) {
		return WEBSOCKET_ROUTES[rpcType];
	}
};