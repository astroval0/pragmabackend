#pragma once

#include <PacketProcessor.h>
#include <Registry.h>

namespace http = boost::beast::http;

class StaticResponseProcessorHTTP : public HTTPPacketProcessor {
protected:
	std::string m_route;
	boost::asio::const_buffer m_res;
public:
	StaticResponseProcessorHTTP(std::string route, boost::asio::const_buffer res) : m_route(route), m_res(std::move(res)) {
		Registry::HTTP_ROUTES[route] = this;
	};

	void Process(http::request<http::string_body> const& req, boost::asio::ip::tcp::socket* sock) override;

	std::string GetRoute() override {
		return m_route;
	}
};

class StaticResponseProcessorWS : public WebsocketPacketProcessor {
protected:
	std::string m_type;
	boost::asio::const_buffer m_res;
public:
	StaticResponseProcessorWS(std::string type, boost::asio::const_buffer res) : m_type(type), m_res(std::move(res)) {
		Registry::WEBSOCKET_ROUTES[type] = this;
	}

	void Process(boost::beast::flat_buffer packet, boost::beast::websocket::stream<boost::asio::ip::tcp::socket>* sock) override;

	std::string GetType() override {
		return m_type;
	}
};