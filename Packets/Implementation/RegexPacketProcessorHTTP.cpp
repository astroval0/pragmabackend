#include <RegexPayloadProcessorHTTP.h>
#include <spdlog/spdlog.h>

void RegexPayloadProcessorHTTP::Process(http::request<http::string_body> const& req, tcp::socket& sock) {
    http::response<http::string_body> res;
    res.version(req.version());
    res.keep_alive(req.keep_alive());
    res.result(http::status::ok);
    res.set(http::field::content_type, "application/json; charset=UTF-8");
    res.set(http::field::vary, "Origin");
    for (const auto& [regex, payload] : m_resMap) {
        if (std::regex_search(req.body(), regex.m_rx)) {
            res.body() = payload->dump();
            res.prepare_payload();
            http::write(sock, res);
            return;
        }
    }
    spdlog::error("Regex processor {} failed to find any valid response for the packet, dropping the packet.\nPacket contents: {}", GetRoute(), req.body());
}