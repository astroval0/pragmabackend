#include "SpectreWebsocketRequest.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

SpectreWebsocketRequest::SpectreWebsocketRequest(SpectreWebsocket& sock, reqbuf req) : 
	m_websocket(sock), m_requestbuf(std::move(req)), m_reqjson(json::parse(static_cast<const char*>(m_requestbuf.data().data()), static_cast<const char*>(m_requestbuf.data().data()) + m_requestbuf.size())) {
	try {
		m_requestType = SpectreRpcType(std::string(m_reqjson["type"]));
	}
	catch(std::exception e) {
		spdlog::warn("log type not found for " + m_reqjson["type"]);
	}
	m_requestId = m_reqjson["requestId"];
}

json SpectreWebsocketRequest::GetBaseJsonResponse() {
	json resJson = json::object();
	std::string resType = m_requestType.GetName();
	resType = std::string(resType.begin(), resType.end() - sizeof("Request") + 1);
	resType += "Response";
	resJson["response"] = json({
		{ "requestId", m_requestId },
		{ "type", resType },
		{ "payload", json::object() }
	});
	return resJson;
}

void SpectreWebsocketRequest::SendEmptyResponse() {
	json resJson = GetBaseJsonResponse();
	m_websocket.SendPacket(resJson);
}