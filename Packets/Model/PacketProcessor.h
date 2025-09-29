#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <SpectreWebsocketRequest.h>
#include <SpectreRpcType.h>

namespace http = boost::beast::http;

class HTTPPacketProcessor {
public:
	virtual void Process(http::request<http::string_body> const& req, boost::asio::ip::tcp::socket* sock) = 0;
	virtual ~HTTPPacketProcessor() = default;
	virtual std::string GetRoute() = 0;
};

class WebsocketPacketProcessor {
public:
	virtual void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) = 0;
	virtual ~WebsocketPacketProcessor() = default;
	virtual const SpectreRpcType& GetType() = 0;
};