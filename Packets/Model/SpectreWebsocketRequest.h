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
	void* m_payload;
	SpectreRpcType m_requestType;
	int m_requestId;
public:
	SpectreWebsocketRequest(SpectreWebsocket& sock, reqbuf req);

	char* GetRequestPayload() {
		return reinterpret_cast<char*>(m_payload);
	}

	SpectreWebsocket& GetSocket() {
		return m_websocket;
	}
	
	SpectreRpcType GetRequestType() {
		return m_requestType;
	}
	
	json GetBaseJsonResponse();
	void SendBasicResponse();
};