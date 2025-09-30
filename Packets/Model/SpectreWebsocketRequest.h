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
using json = nlohmann::json;

class SpectreWebsocketRequest {
private:
	SpectreWebsocket& m_websocket;
	reqbuf m_requestbuf;
	json* m_payload;
	SpectreRpcType m_requestType;
	int m_requestId;
public:
	SpectreWebsocketRequest(SpectreWebsocket& sock, reqbuf req);

	json* GetRequestPayload() {
		return m_payload;
	}

	SpectreWebsocket& GetSocket() {
		return m_websocket;
	}
	
	SpectreRpcType GetRequestType() {
		return m_requestType;
	}
	
	json GetBaseJsonResponse();
	void SendEmptyResponse();
};