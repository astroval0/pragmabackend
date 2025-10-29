#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <SpectreRpcType.h>
#include <nlohmann/json.hpp>
#include <SpectreWebsocket.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>

namespace pbuf = google::protobuf;
using reqbuf = boost::beast::flat_buffer;

class SpectreWebsocketRequest {
private:
	SpectreWebsocket& m_websocket;
	reqbuf m_requestbuf;
	SpectreRpcType m_requestType;
	std::shared_ptr<json> m_reqjson;
	std::string m_payloadAsStr;
	int m_requestId;
public:
	SpectreWebsocketRequest(SpectreWebsocket& sock, reqbuf req);

	std::shared_ptr<json> GetPayload();

	template<typename T> 
	std::unique_ptr<T> GetPayloadAsMessage() {
		static_assert(std::is_base_of<pbuf::Message, T>::value, "Type passed to GetPayloadAsMessage must be subclass of pbuf::Message");
		T message;
		auto status = pbuf::util::JsonStringToMessage(
			m_payloadAsStr,
			&message
		);
		if (!status.ok()) {
			spdlog::error("Failed to parse incoming request to message: {}", status.message());
			throw;
		}
		return std::make_unique<T>(message);
	}

	reqbuf* GetRawBuffer() {
		return &m_requestbuf;
	}

	SpectreWebsocket& GetSocket() {
		return m_websocket;
	}
	
	SpectreRpcType GetRequestType() {
		return m_requestType;
	}

	std::string GetResponseType();

	int GetRequestId() {
		return m_requestId;
	}
	
	std::shared_ptr<json> GetBaseJsonResponse();
	void SendEmptyResponse();
};