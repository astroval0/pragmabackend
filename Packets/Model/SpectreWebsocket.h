#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <SpectreRpcType.h>
#include <nlohmann/json.hpp>

using ws = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;
using json = nlohmann::json;

class SpectreWebsocket {
private:
	ws& socket;
	int curSequenceNumber;
public:
	SpectreWebsocket(ws& sock) : socket(sock), curSequenceNumber(0) {};
	/* 
		Warning: Do not send packets through the socket directly, it bypasses abstraction and will cause bad things to happen
	*/
	ws& GetRawSocket() const {
		return socket;
	}

	void SendPacket(json& res) {
		res["sequenceNumber"] = curSequenceNumber;
		curSequenceNumber++;
		socket.text(true);
		socket.write(boost::asio::buffer(res.dump()));
	}
};