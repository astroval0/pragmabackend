#pragma once

#include <PacketProcessor.h>

namespace http = boost::beast::http;

class StaticResponseProcessorHTTP : public HTTPPacketProcessor {
private:
	boost::asio::const_buffer m_res;
public:
	StaticResponseProcessorHTTP(std::string route, boost::asio::const_buffer res) : HTTPPacketProcessor(route), m_res(std::move(res)) {};

	void Process(http::request<http::string_body> const& req, tls_stream& sock) override;
};

class StaticResponseProcessorWS : public WebsocketPacketProcessor {
private:
	json& m_res;
public:
	StaticResponseProcessorWS(SpectreRpcType& type, json& res) : WebsocketPacketProcessor(type), m_res(res) {};

	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};