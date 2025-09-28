#include <StaticResponseProcessor.h>

void StaticResponseProcessorHTTP::Process(http::request<http::string_body> const& req, boost::asio::ip::tcp::socket* sock) {
	http::response<http::string_body> res;
	res.set(http::field::server, "FakePragma");
	res.set(http::field::content_type, "application/json");
	res.result(http::status::ok);
	res.body() = reinterpret_cast<const char*>(m_res.data());
	sock->send(m_res);
}

void StaticResponseProcessorWS::Process(boost::beast::flat_buffer packet, boost::beast::websocket::stream<boost::asio::ip::tcp::socket>* sock) {
	sock->text(true);
	sock->write(m_res);
}