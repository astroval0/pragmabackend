#include "SpectreWebsocketRequest.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

SpectreWebsocketRequest::SpectreWebsocketRequest(SpectreWebsocket& sock, reqbuf req) : 
	m_websocket(sock), m_requestbuf(std::move(req)) {
	const char* jsonbuf = static_cast<const char*>(m_requestbuf.data().data());
	json requestJson = json::parse(jsonbuf, jsonbuf + m_requestbuf.size());
	try {
		m_requestType = SpectreRpcType(std::string(requestJson["type"]));
	}
	catch(std::exception e) {
		spdlog::warn("log type not found for " + requestJson["type"]);
	}
	m_requestId = requestJson["requestId"];
	m_payload = &requestJson["payload"];
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