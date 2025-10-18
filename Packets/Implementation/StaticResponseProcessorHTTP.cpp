#include <StaticResponseProcessorHTTP.h>

void StaticResponseProcessorHTTP::Process(http::request<http::string_body> const& req, tls_stream& sock) {
    http::response<http::string_body> res;
    res.version(req.version());
    res.keep_alive(req.keep_alive());
    res.result(http::status::ok);
    res.set(http::field::content_type, "application/json; charset=UTF-8");
    res.set(http::field::vary, "Origin");
    res.body() = m_res->dump();
    res.prepare_payload();
    http::write(sock, res);
}