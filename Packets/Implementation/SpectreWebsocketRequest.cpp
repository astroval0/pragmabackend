#include "SpectreWebsocketRequest.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

SpectreWebsocketRequest::SpectreWebsocketRequest(SpectreWebsocket& sock, reqbuf req) : 
	m_websocket(sock), m_requestbuf(std::move(req)) {
	json reqjson = json::parse(static_cast<const char*>(m_requestbuf.data().data()), static_cast<const char*>(m_requestbuf.data().data()) + m_requestbuf.size());
	m_reqjson = std::make_shared<json>(reqjson);
	try {
		m_requestType = SpectreRpcType((*m_reqjson)["type"].get<std::string>());
	}
	catch(std::exception e) {
		std::string type_str;
		try
		{
			if (m_reqjson->contains("type"))
			{
				if ((*m_reqjson)["type"].is_string()) type_str = (*m_reqjson)["type"];
				else type_str = (*m_reqjson)["type"].dump();
			}
			else {
				type_str = "<missing>";
			}
		}
		catch (...) {
			type_str = "<unreadable>";
		}
		spdlog::error("Received websocket request with invalid type field: {}. Exception: {}", type_str, e.what());
	}
	m_requestId = (*m_reqjson)["requestId"];
	m_payloadAsStr = (*m_reqjson)["payload"].dump();
}

std::shared_ptr<json> SpectreWebsocketRequest::GetPayload() {
	return std::make_shared<json>(((*m_reqjson)["payload"]));
}

std::shared_ptr<json> SpectreWebsocketRequest::GetBaseJsonResponse() {
	json response;
	response["requestId"] = m_requestId;
	response["type"] = GetResponseType();
	response["payload"] = json::object();
    return std::make_shared<json>(std::move(response));
}

void SpectreWebsocketRequest::SendEmptyResponse() {
	m_websocket.SendPacket(GetBaseJsonResponse());
}

std::string SpectreWebsocketRequest::GetResponseType() {
	std::string resType = m_requestType.GetName();
	if (resType.size() >= 7 && resType.compare(resType.size() - 7, 7, "Request") == 0)
		resType.erase(resType.size() - 7);
	resType += "Response";
	return resType;
}