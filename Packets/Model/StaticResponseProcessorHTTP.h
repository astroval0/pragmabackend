#pragma once

#include <PacketProcessor.h>

namespace http = boost::beast::http;

class StaticResponseProcessorHTTP : public HTTPPacketProcessor {
private:
    std::shared_ptr<json> m_res;
public:
    StaticResponseProcessorHTTP(std::string route, Site site, std::shared_ptr<json> res) : HTTPPacketProcessor(route, site), m_res(res) {};

    void Process(http::request<http::string_body> const& req, tls_stream& sock) override;
};