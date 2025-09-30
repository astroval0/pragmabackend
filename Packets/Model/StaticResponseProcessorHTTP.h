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