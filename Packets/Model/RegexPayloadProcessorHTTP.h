#pragma once
#include <PacketProcessor.h>
#include <unordered_map>
#include "Regex.h"

class RegexPayloadProcessorHTTP : public HTTPPacketProcessor {
private:
    std::unordered_map<regex, std::shared_ptr<json>> m_resMap;
public:
    RegexPayloadProcessorHTTP(std::string route, std::unordered_map<regex, std::shared_ptr<json>> resMap)
        : HTTPPacketProcessor(route), m_resMap(resMap) {};

    void Process(http::request<http::string_body> const& req, tcp::socket& sock) override;
};