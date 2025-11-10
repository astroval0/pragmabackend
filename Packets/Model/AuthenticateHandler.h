#pragma once
#include <PacketProcessor.h>

class AuthenticateHandler : public HTTPPacketProcessor {
public:
	explicit AuthenticateHandler(std::string route);
	void Process(http::request<http::string_body> const& req, tcp::socket& sock) override;
private:
	std::string CreatePlayerFromSteam(const std::string& steam64, const std::string& displayName);
	std::string BuildJwt(
		const std::string& backendType,
		const std::string& playerId,
		const std::string& socialId,
		const std::string& displayName,
		const std::string& discriminator
	);
};