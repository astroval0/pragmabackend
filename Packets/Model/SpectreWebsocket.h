#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <SpectreRpcType.h>
#include <nlohmann/json.hpp>
#include <google/protobuf/message.h>

using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace http = boost::beast::http;
using ws = boost::beast::websocket::stream<tcp::socket>;
using json = nlohmann::ordered_json;
namespace pbuf = google::protobuf;

class SpectreWebsocket {
private:
	ws& socket;
	int curSequenceNumber;
	std::string m_playerId;
public:
	SpectreWebsocket(ws& sock, const http::request<http::string_body>& req);
	/* 
		Warning: Do not send packets through the socket directly, it bypasses abstraction and will cause bad things to happen
	*/
	const ws& GetRawSocket();

	void SendPacket(std::shared_ptr<json> res);

	void SendPacket(const std::string& resPayload, int requestId, const std::string& resType);

	void SendPacket(const pbuf::Message& res, const std::string& resType, int requestId);

	void SendNotification(std::shared_ptr<json> notif, const SpectreRpcType& notificationType);

	void SendNotification(const std::string& notifPayload, const SpectreRpcType& notificationType);

	void SendNotification(const pbuf::Message& notif, const SpectreRpcType& notificationType);

	const std::string& GetPlayerId();
};