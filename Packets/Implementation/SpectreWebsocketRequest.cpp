#include "SpectreWebsocketRequest.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

SpectreWebsocketRequest::SpectreWebsocketRequest(SpectreWebsocket& sock, reqbuf req) : 
	m_websocket(sock), m_requestbuf(std::move(req)) {

	json requestJson = json::parse(GetRequestPayload());
	m_requestType = SpectreRpcType(std::string(requestJson["type"]));
	m_requestId = requestJson["requestId"];
	m_payload = &requestJson["payload"];
}

json SpectreWebsocketRequest::GetBaseJsonResponse() {
	json resJson;
	resJson["type"] = m_requestType.GetName();
	std::string resType = m_requestType.GetName();
	resType = std::string(resType.begin(), resType.end() - sizeof("Request"));
	resType += "Response";
	resJson["response"] = {
		{ "requestId", m_requestId },
		{ "type", resType },
		{ "payload" }
	};
	return resJson;
}

void SpectreWebsocketRequest::SendBasicResponse() {
	json resJson = GetBaseJsonResponse();
	m_websocket.SendPacket(resJson);
}