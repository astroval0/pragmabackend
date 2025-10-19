#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <SpectreRpcType.h>
#include <nlohmann/json.hpp>
#include <SpectreWebsocket.h>

using reqbuf = boost::beast::flat_buffer;

class SpectreWebsocketRequest {
private:
	SpectreWebsocket& m_websocket;
	reqbuf m_requestbuf;
	SpectreRpcType m_requestType;
	std::shared_ptr<json> m_reqjson;
	int m_requestId;
public:
	SpectreWebsocketRequest(SpectreWebsocket& sock, reqbuf req);

	std::shared_ptr<json> GetPayload() {
		return std::make_shared<json>(((*m_reqjson)["payload"]));
	}

	SpectreWebsocket& GetSocket() {
		return m_websocket;
	}
	
	SpectreRpcType GetRequestType() {
		return m_requestType;
	}
	
	std::shared_ptr<json> GetBaseJsonResponse();
	void SendEmptyResponse();
};