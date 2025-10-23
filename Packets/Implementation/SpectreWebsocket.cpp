#include <SpectreWebsocket.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>

pbuf::util::JsonPrintOptions opts;

SpectreWebsocket::SpectreWebsocket(ws& sock, const http::request<http::string_body>& req) : socket(sock), curSequenceNumber(0)
{
	socket.auto_fragment(false); // ideally we dont want to have to use this but we should be fine for now
	// TODO parse the req into a playerID from the bearer token
	m_playerId = "1";
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
	std::string finalRes = "{\"sequenceNumber\":" + std::to_string(curSequenceNumber) 
		+ ",\"response\":{\"requestId\":" + std::to_string(requestId) 
		+ ",\"type\":\"" + resType + "\",\"payload\":";
	std::string resComponent;
	if (!pbuf::util::MessageToJsonString(payload, &resComponent, opts).ok()) {
		spdlog::error("Failed to serialize pbuf message to string in SendPacket");
		throw;
	}
	finalRes += resComponent + "}";
	curSequenceNumber++;
	socket.text(true);
	socket.write(boost::asio::buffer(finalRes));
}

const std::string& SpectreWebsocket::GetPlayerId() {
	return m_playerId;
}