#include <AuthenticateHandler.h>
#include <PlayerDatabase.h>
#include <SteamValidator.h>
#include <AuthLatch.h>
#include <ProfileData.pb.h>
#include <spdlog/spdlog.h>


static json LoadAuthConfig() {
	std::ifstream f("auth.json");
	if (!f.is_open()) throw std::runtime_error("failed to open auth.json");
	return json::parse(f);
}

AuthenticateHandler::AuthenticateHandler(std::string route) : HTTPPacketProcessor(std::move(route)) {}

static std::string client_ip(const tcp::socket& sock) {
	try {
		return sock.remote_endpoint().address().to_string();
	}
	catch (...) {
		return "0.0.0.0";
	}
}

void AuthenticateHandler::Process(http::request<http::string_body> const& req, tcp::socket& sock) {
	http::response<http::string_body> res;
	res.version(req.version());
	res.keep_alive(req.keep_alive());
	res.set(http::field::content_type, "application/json; charset=UTF-8");
	res.set(http::field::vary, "Origin");
	
	const auto& route = GetRoute();
	try {
		auto cfg = LoadAuthConfig();
		const auto steamKey = cfg["steamApiKey"].get<std::string>();
		
		if (route == "/v1/submitproviderid") {
			auto body = json::parse(req.body());
			if (!body.contains("providerId")) {
				res.result(http::status::bad_request);
				res.body() = R"({"error":"missing providerId"})";
				res.prepare_payload();
				http::write(sock, res);
				return;
			}
			const std::string steam64 = body["providerId"].get<std::string>();

			SteamValidator v(steamKey);
			auto info = v.ValidateSteamId(steam64);
			if (!info) {
				res.result(http::status::bad_request);
				res.body() = R"({"error":"invalid steam id"})";
				res.prepare_payload();
				http::write(sock, res);
				return;
			}

			AuthLatch::Get().Put(client_ip(sock), steam64, 5);
			res.result(http::status::ok);
			res.body() = R"({"ok":true})";
			res.prepare_payload();
			http::write(sock, res);
			return;
		}

		if (route == "/v1/account/authenticateorcreatev2") {
			auto ip = client_ip(sock);
			auto steam64 = AuthLatch::Get().TakeIfFresh(ip);
			if (steam64.empty()) {
				res.result(http::status::bad_request);
				res.body() = R"({"error":"NOSTEAMID"})";
				res.prepare_payload();
				http::write(sock, res);
				return;
			}

			auto& db = PlayerDatabase::Get();
			auto playerId = db.LookupPlayerByProvider("STEAM", steam64);

			if (playerId.empty()) {
				SteamValidator v(steamKey);
				auto info = v.ValidateSteamId(steam64);
				if (!info) {
					res.result(http::status::bad_request);
					res.body() = R"({"error":"invalid steam id"})";	
					res.prepare_payload();
					http::write(sock, res);
					return;
				}
				playerId = CreatePlayerFromSteam(steam64, info->personaName);
				db.UpsertProviderMap("STEAM", steam64, playerId);
			}

			if (db.IsBanned(playerId)) {
				res.result(http::status::forbidden);
				res.body() = R"({"error":"ACCOUNT BANNED. CONTACT ASTROVAL0 ON DISCORD"})";
				res.prepare_payload();
				http::write(sock, res);
				return;
			}

			auto prof = db.GetField<ProfileData>(FieldKey::PLAYER_DATA, playerId);
			std::string display = prof ? prof->displayname().displayname() : "Player";
			std::string disc = prof ? prof->displayname().discriminator() : "0000";
			const std::string socialId = playerId; // TODO implement socialid; idk for now we just use playerId as socialId 

			auto tokens = json::object();
			tokens["pragmaGameToken"] = BuildJwt("GAME", playerId, socialId, display, disc);
			tokens["pragmaSocialToken"] = BuildJwt("SOCIAL", playerId, socialId, display, disc);

			json out = { {"pragmaTokens", tokens} };
			res.result(http::status::ok);
			res.body() = out.dump();
			res.prepare_payload();
			http::write(sock, res);
			return;
		}

		res.result(http::status::not_found);
		res.body() = R"({"error":"no fucking idea"})";
		res.prepare_payload();
		http::write(sock, res);
	}
	catch (const std::exception& e) {
		res.result(http::status::internal_server_error);
		res.body() = R"({"error":"internal server error"})";
		res.prepare_payload();
		http::write(sock, res);
	}
}

std::string AuthenticateHandler::CreatePlayerFromSteam(const std::string& steam64, const std::string& displayName) {
	static const char* hex = "0123456789abcdef";
	std::string uuid; uuid.reserve(36);
	std::mt19937 rng{ std::random_device{}() };
	std::uniform_int_distribution<int> dist(0, 15);
	for (int i = 0; i < 36; ++i) {
		if (i == 8 || i == 13 || i == 18 || i == 23) { uuid.push_back('-'); continue; }
		if (i == 14) { uuid.push_back('4'); continue; }
		if (i == 19) { uuid.push_back(hex[(dist(rng) & 0x3) | 0x8]); continue; }
		uuid.push_back(hex[dist(rng)]);
	}

	ProfileData pd;
	pd.set_playerid(uuid);
	auto* dn = pd.mutable_displayname();
	dn->set_displayname(displayName.empty() ? "Player" : displayName);
	auto n = std::uniform_int_distribution<int>(0, 9999)(rng);
	char disc[5]; 
	snprintf(disc, 5, "%04d", n);
	dn->set_discriminator(disc);

	PlayerDatabase::Get().SetField(FieldKey::PLAYER_DATA, &pd, uuid);

	return uuid;
}

static std::string b64url_json(const nlohmann::json& j) {
	const std::string s = j.dump();
	static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	std::string out;
	out.reserve(((s.size() + 2) / 3) * 4);
	int val = 0, valb = -6;
	for (unsigned char c : s) { 
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(t[(val >> valb) & 0x3F]); 
			valb -= 6;
		}
	}

	if (valb > -6) out.push_back(t[((val << 8) >> (valb + 8)) & 0x3F]);

	while (out.size() % 4) out.push_back('=');
	while (!out.empty() && out.back() == '=') out.pop_back();
	return out;
}

std::string AuthenticateHandler::BuildJwt(
	const std::string& backendType,
	const std::string& playerId,
	const std::string& socialId,
	const std::string& displayName,
	const std::string& discriminator
) {
	const auto now = static_cast<long long>(time(nullptr));
	const auto exp = now + 24 * 3600; // 24 hrs 

	nlohmann::json header = {
		{"alg", "none"}, // idk if we should be signing tokens so if teh game rejects auth response then we know.
		{"typ", "JWT"}
	};

	nlohmann::json payload = {
		{"iss","pragma"},
		{"sub", backendType == "GAME" ? playerId : socialId},
		{"iat", now},
		{"exp", exp},
		{"jti", playerId},
		{"sessionType","PLAYER"},
		{"backendType", backendType},
		{"displayName", displayName},
		{"discriminator", discriminator},
		{"pragmaSocialId", socialId},
		{"idProvider","STEAM"},
		{"pragmaPlayerId", playerId}
	};

	if (backendType == "GAME") {
		payload["gameShardId"] = "00000000-0000-0000-0000-000000000001";
	}

	return b64url_json(header) + "." + b64url_json(payload) + ".";
}