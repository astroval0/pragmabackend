#include <SteamValidator.h>
#include <spdlog/spdlog.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
using json = nlohmann::json;

SteamValidator::SteamValidator(std::string apiKey) : m_apiKey(std::move(apiKey)) {}

std::string SteamValidator::HttpGet(const std::string& host, const std::string& target) {
	boost::asio::io_context ioc;
	tcp::resolver resolver{ioc};

	boost::beast::tcp_stream stream{ ioc };
	auto const results = resolver.resolve(host, "80");
	stream.connect(results);
	http::request<http::string_body> req{ http::verb::get, target, 11 };
	req.set(http::field::host, host);
	http::write(stream, req);
	boost::beast::flat_buffer buffer;
	http::response<http::string_body> res;
	http::read(stream, buffer, res);
	boost::system::error_code ec;
	stream.socket().shutdown(tcp::socket::shutdown_both, ec);
	return res.body();
}

std::optional<SteamPlayerInfo> SteamValidator::ValidateSteamId(const std::string& steam64) {
	if (m_apiKey.empty()) return std::nullopt;

	auto body = HttpGet("api.steampowered.com",
		"/ISteamUser/GetPlayerSummaries/v2/?key=" + m_apiKey + "&steamids=" + steam64 /* + "&format=json"*/);

	try {
		auto j = json::parse(body);
		auto& players = j["response"]["players"];

		if (players.empty()) return std::nullopt;
		SteamPlayerInfo out;
		auto& player = players[0];
		out.steamId = player.value("steamid", "");
		out.personaName = player.value("personaname", "");
		return out;
	} catch (const std::exception& e) {
		spdlog::error("Failed to parse Steam API response: {}", e.what());
		return std::nullopt;
	}
}