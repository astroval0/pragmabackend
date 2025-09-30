#include <StaticResponseProcessor.h>

void StaticResponseProcessorHTTP::Process(http::request<http::string_body> const& req, tls_stream& sock) {
	http::response<http::string_body> res;
	res.set(http::field::server, "FakePragma");
	res.set(http::field::content_type, "application/json");
	res.result(http::status::ok);
	res.body() = reinterpret_cast<const char*>(m_res.data());
	res.prepare_payload();
	http::write(sock, res);
}

void StaticResponseProcessorWS::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	sock.SendPacket(m_res);
}