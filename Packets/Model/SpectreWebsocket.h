#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <SpectreRpcType.h>
#include <nlohmann/json.hpp>

using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
using tls_stream = ssl::stream<tcp::socket>;
using ws = boost::beast::websocket::stream<tls_stream>;
using json = nlohmann::json;

class SpectreWebsocket {
private:
	ws& socket;
	int curSequenceNumber;
public:
	SpectreWebsocket(ws& sock) : socket(sock), curSequenceNumber(0)
	{
		socket.auto_fragment(false); // ideally we dont want to have to use this but we should be fine for now
	};
	/* 
		Warning: Do not send packets through the socket directly, it bypasses abstraction and will cause bad things to happen
	*/
	ws& GetRawSocket() const {
		return socket;
	}

	void SendPacket(std::shared_ptr<json> res) {
		(*res)["sequenceNumber"] = curSequenceNumber;
		curSequenceNumber++;
		socket.text(true);
		//dont pass temporary because beast will fragment it
		std::string msg = res->dump();
		socket.write(boost::asio::buffer(msg));
	}
};