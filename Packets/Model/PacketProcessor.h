#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>

namespace http = boost::beast::http;

class HTTPPacketProcessor {
public:
	virtual void Process(http::request<http::string_body> const& req, boost::asio::ip::tcp::socket* sock) = 0;
	virtual ~HTTPPacketProcessor() = default;
	virtual std::string GetRoute() = 0;
};

class WebsocketPacketProcessor {
public:
	virtual void Process(boost::beast::flat_buffer packet, boost::beast::websocket::stream<boost::asio::ip::tcp::socket>* sock) = 0;
	virtual ~WebsocketPacketProcessor() = default;
	virtual std::string GetType() = 0;
};