#include <SpectreWebsocket.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>
#include "../../vcpkg/buildtrees/jwt-cpp/src/v0.7.1-fe911b1e8a.clean/include/jwt-cpp/traits/nlohmann-json/traits.h"

pbuf::util::JsonPrintOptions opts = []() {
	pbuf::util::JsonPrintOptions options;
	options.always_print_fields_with_no_presence = true;
	return options;
	}();

static std::string extract_bearer(const http::request<http::string_body>& req) {
	auto auth = req.base()[http::field::authorization];
	if (auth.empty()) return {};
	static constexpr char prefix[] = "Bearer ";
	const std::string s = std::string(auth);
	if (s.rfind(prefix, 0) != 0) return {};
	return s.substr(sizeof(prefix) - 1);
}

static std::string decode_player_id_noverify(const std::string& token) {
	try {
		const auto decoded = jwt::decode<jwt::traits::nlohmann_json>(token);

		if (decoded.has_payload_claim("pragmaPlayerId")) {
			return decoded.get_payload_claim("pragmaPlayerId").as_string();
		}

		try {
			return decoded.get_subject();
		}
		catch (...) {
			if (decoded.has_payload_claim("sub")) {
				return decoded.get_payload_claim("sub").as_string();
			}
		}
	}
	catch (const std::exception& e) {
		spdlog::warn("JWT decode failed: {}", e.what());
	}
	return {};
}

SpectreWebsocket::SpectreWebsocket(ws& sock, const http::request<http::string_body>& req) : socket(sock), curSequenceNumber(0)
{
	socket.auto_fragment(false); // ideally we dont want to have to use this but we should be fine for now
	const auto bearer = extract_bearer(req);
	const auto pid = bearer.empty() ? std::string() : decode_player_id_noverify(bearer);

	if (!pid.empty()) {
		m_playerId = pid;
	}
	else {
		spdlog::error("no playerid ???? investigate me!");
		m_playerId = "1";
	}
};

const ws& SpectreWebsocket::GetRawSocket() {
	return socket;
}

void SpectreWebsocket::SendPacket(std::shared_ptr<json> res) {
	json packet;
	packet["sequenceNumber"] = curSequenceNumber;
	packet["response"] = *res;
	curSequenceNumber++;
	socket.text(true);
	//dont pass temporary because beast will fragment it
	std::string msg = packet.dump();
	socket.write(boost::asio::buffer(msg));
}

void SpectreWebsocket::SendPacket(const pbuf::Message& payload, const std::string& resType, int requestId) {
	// you shan't comment on this cursedness
	std::string finalRes = "{\"sequenceNumber\":" + std::to_string(curSequenceNumber)
		+ ",\"response\":{\"requestId\":" + std::to_string(requestId)
		+ ",\"type\":\"" + resType + "\",\"payload\":";
	std::string resComponent;
	if (!pbuf::util::MessageToJsonString(payload, &resComponent, opts).ok()) {
		spdlog::error("Failed to serialize pbuf message to string in SendPacket");
		throw;
	}
	finalRes += resComponent + "}}";
	curSequenceNumber++;
	socket.text(true);
	socket.write(boost::asio::buffer(finalRes));
}

void SpectreWebsocket::SendPacket(const std::string& resPayload, int requestId, const std::string& resType) {
	std::string finalRes = "{\"sequenceNumber\":" + std::to_string(curSequenceNumber)
		+ ",\"response\":{\"requestId\":" + std::to_string(requestId)
		+ ",\"type\":\"" + resType + "\",\"payload\":";
	finalRes += resPayload + "}}";
	curSequenceNumber++;
	socket.text(true);
	socket.write(boost::asio::buffer(finalRes));
}

void SpectreWebsocket::SendNotification(std::shared_ptr<json> notifPayload, const SpectreRpcType& notificationType) {
	json packet;
	packet["sequenceNumber"] = curSequenceNumber;
	packet["notification"]["type"] = notificationType.GetName();
	packet["notification"]["payload"] = *notifPayload;
	curSequenceNumber++;
	socket.text(true);
	std::string msg = packet.dump();
	socket.write(boost::asio::buffer(msg));
}

void SpectreWebsocket::SendNotification(const std::string& notifPayload, const SpectreRpcType& notificationType) {
	std::string finalPayload = "{\"sequenceNumber\":" + std::to_string(curSequenceNumber)
		+ ",\"notification\":{\"type\":\"" + notificationType.GetName() + "\",\"payload\":"
		+ notifPayload + "}}";
	curSequenceNumber++;
	socket.text(true);
	socket.write(boost::asio::buffer(finalPayload));
}

void SpectreWebsocket::SendNotification(const pbuf::Message& notif, const SpectreRpcType& notificationType) {
	std::string finalRes = "{\"sequenceNumber\":" + std::to_string(curSequenceNumber)
		+ ",\"notification\":\"type\":\"" + notificationType.GetName() + "\",\"payload\":";
	std::string payloadComponent;
	if (!pbuf::util::MessageToJsonString(notif, &payloadComponent, opts).ok()) {
		spdlog::error("Failed to serialize pbuf message to string in SendPacket");
		throw;
	}
	finalRes += payloadComponent + "}}";
	curSequenceNumber++;
	socket.text(true);
	socket.write(boost::asio::buffer(finalRes));
}

const std::string& SpectreWebsocket::GetPlayerId() {
	return m_playerId;
}