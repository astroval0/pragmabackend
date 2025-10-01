#include "SpectreWebsocketRequest.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

SpectreWebsocketRequest::SpectreWebsocketRequest(SpectreWebsocket& sock, reqbuf req) : 
	m_websocket(sock), m_requestbuf(std::move(req)) {
	json reqjson = json::parse(static_cast<const char*>(m_requestbuf.data().data()), static_cast<const char*>(m_requestbuf.data().data()) + m_requestbuf.size());
	m_reqjson = std::make_shared<json>(reqjson);
	try {
		m_requestType = SpectreRpcType(std::string((*m_reqjson)["type"]));
	}
	catch(std::exception e) {
		spdlog::warn("log type not found for " + (*m_reqjson)["type"]);
	}
	m_requestId = (*m_reqjson)["requestId"];
}

std::shared_ptr<json> SpectreWebsocketRequest::GetBaseJsonResponse() {
    json out = json::object();
    out["sequenceNumber"] = 0;
    std::string resType = m_requestType.GetName();
    if (resType.size() >= 7 && resType.compare(resType.size() - 7, 7, "Request") == 0)
        resType.erase(resType.size() - 7);
    resType += "Response";
    out["response"] = json{
        {"requestId", m_requestId},
        {"type",      resType},
        {"payload",   json::object()}
    };
    return std::make_shared<json>(std::move(out));
}

void SpectreWebsocketRequest::SendEmptyResponse() {
	m_websocket.SendPacket(GetBaseJsonResponse());
}