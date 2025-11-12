#include <AuthenticateHandler.h>
#include <PlayerDatabase.h>
#include <SteamValidator.h>
#include <AuthLatch.h>
#include <ProfileData.pb.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/name_generator_sha1.hpp>
#include <boost/uuid/random_generator.hpp>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <ctime>
#include <cstdio>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#if defined(_WIN32)
extern "C" {
#  include <openssl/applink.c>
}
#endif

using tcp = boost::asio::ip::tcp;

static std::string read_all(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("open failed: " + path);
    f.seekg(0, std::ios::end);
    std::string s;
    s.resize(static_cast<size_t>(f.tellg()));
    f.seekg(0, std::ios::beg);
    f.read(s.data(), static_cast<std::streamsize>(s.size()));
    return s;
}

static std::string b64url_bytes(const unsigned char* data, size_t len) {
    static constexpr char t[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    int val = 0, valb = -6;
    for (size_t i = 0; i < len; ++i) {
        val = (val << 8) + data[i];
        valb += 8;
        while (valb >= 0) {
            out.push_back(t[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(t[((val << 8) >> (valb + 8)) & 0x3F]);
    return out;
}

static std::string sign_rs256_b64url(const std::string& signing_input) {
    const std::string pem = read_all(std::string(CERT_DIR) + "/jwt/pragma_private.pem"); // simply just the JWT priv key.

    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio) throw std::runtime_error("BIO_new_mem_buf failed");
    std::unique_ptr<BIO, int(*)(BIO*)> bio_u(bio, BIO_free);

    EVP_PKEY* pkey_raw = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    if (!pkey_raw) throw std::runtime_error("PEM_read_bio_PrivateKey failed");
    std::unique_ptr<EVP_PKEY, void(*)(EVP_PKEY*)> pkey(pkey_raw, EVP_PKEY_free);

    EVP_MD_CTX* ctx_raw = EVP_MD_CTX_new();
    if (!ctx_raw) throw std::runtime_error("EVP_MD_CTX_new failed");
    std::unique_ptr<EVP_MD_CTX, void(*)(EVP_MD_CTX*)> ctx(ctx_raw, EVP_MD_CTX_free);

    if (EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) != 1)
        throw std::runtime_error("EVP_DigestSignInit failed");
    if (EVP_DigestSignUpdate(ctx.get(), signing_input.data(), signing_input.size()) != 1)
        throw std::runtime_error("EVP_DigestSignUpdate failed");

    size_t siglen = 0;
    if (EVP_DigestSignFinal(ctx.get(), nullptr, &siglen) != 1)
        throw std::runtime_error("EVP_DigestSignFinal(size) failed");

    std::vector<unsigned char> sig(siglen);
    if (EVP_DigestSignFinal(ctx.get(), sig.data(), &siglen) != 1)
        throw std::runtime_error("EVP_DigestSignFinal(data) failed");
    sig.resize(siglen);

    return b64url_bytes(sig.data(), sig.size());
}

struct AuthCfg {
    std::string steamApiKey;
};

static const AuthCfg& GetAuthCfg() {
    static AuthCfg cfg = [] {
        AuthCfg c{};
        auto try_path = [&](const char* p) {
            if (std::ifstream f(p); f.is_open()) {
                auto j = json::parse(f, nullptr, false);
                if (!j.is_discarded() && j.contains("steamApiKey") && j["steamApiKey"].is_string()) {
                    c.steamApiKey = j["steamApiKey"].get<std::string>();
                }
            }
        };
        try_path("auth.json");
        return c;
    }();
    return cfg;
}

AuthenticateHandler::AuthenticateHandler(std::string route) : HTTPPacketProcessor(std::move(route)) {}

static std::string client_ip(const tcp::socket& sock) {
    // just gonna let this throw; ends up 500 anyway
    return sock.remote_endpoint().address().to_string();
}

static std::string PlayerUuidFromSteam64(const std::string& steam64) {
    static const auto ns = boost::uuids::string_generator{}("c8a6b6ce-1e7b-49f2-9a4f-0be3d7b7e5a1");
    const auto id = boost::uuids::name_generator_sha1{ ns }(steam64);
    return boost::lexical_cast<std::string>(id);
}

void AuthenticateHandler::Process(http::request<http::string_body> const& req, tcp::socket& sock) {
    http::response<http::string_body> res;
    res.version(req.version());
    res.keep_alive(req.keep_alive());
    res.set(http::field::content_type, "application/json; charset=UTF-8");
    res.set(http::field::vary, "Origin");

    const auto& route = GetRoute();

    auto reply = [&](http::status st, std::string body) {
        res.result(st);
        res.body() = std::move(body);
        res.prepare_payload();
        http::write(sock, res);
    };

    try {
        const std::string& steamKey = GetAuthCfg().steamApiKey;

        if (route == "/v1/submitproviderid") {
            if (req.method() != http::verb::post) {
                res.set(http::field::allow, "POST");
                return reply(http::status::method_not_allowed, R"({"error":"method not allowed"})");
            }
            if (steamKey.empty()) {
                return reply(http::status::bad_request, R"({"error":"steamApiKey not configured"})");
            }

            const auto body = json::parse(req.body(), nullptr, false);
            if (body.is_discarded() || !body.contains("providerId") || !body["providerId"].is_string()) {
                return reply(http::status::bad_request, R"({"error":"providerId required"})");
            }
            const std::string steam64 = body["providerId"];
            if (steam64.empty()) {
                return reply(http::status::bad_request, R"({"error":"providerId required"})");
            }

            SteamValidator v(steamKey);
            auto info = v.ValidateSteamId(steam64);
            if (!info) {
                return reply(http::status::bad_request, R"({"error":"invalid steam id"})");
            }

            AuthLatch::Get().Put(client_ip(sock), steam64, /*latch timer in seconds*/ 120); //120s for now until i sort the launcher out - astro
            return reply(http::status::ok, R"({"ok":true})");
        }

        if (route == "/v1/account/authenticateorcreatev2") {
            if (req.method() != http::verb::post) {
                res.set(http::field::allow, "POST");
                return reply(http::status::method_not_allowed, R"({"error":"method not allowed"})");
            }

            const auto ip = client_ip(sock);
            const auto steam64 = AuthLatch::Get().TakeIfFresh(ip);
            if (steam64.empty()) {
                return reply(http::status::bad_request, R"({"error":"NOSTEAMID"})");
            }

            auto& db = PlayerDatabase::Get();
            auto playerId = db.LookupPlayerByProvider("STEAM", steam64);

            if (playerId.empty()) {
                std::string persona = "Player";
                if (!steamKey.empty()) {
                    SteamValidator v(steamKey);
                    if (auto info = v.ValidateSteamId(steam64)) persona = info->personaName;
                }
                playerId = CreatePlayerFromSteam(steam64, persona);
                db.UpsertProviderMap("STEAM", steam64, playerId);
            }

            if (db.IsBanned(playerId)) {
                return reply(http::status::forbidden, R"({"error":"ACCOUNT BANNED. CONTACT ASTROVAL0 ON DISCORD"})");
            }

            auto prof = db.GetField<ProfileData>(FieldKey::PROFILE_DATA, playerId);
            const std::string display = prof ? prof->displayname().displayname() : "Player";
            const std::string disc = prof ? prof->displayname().discriminator() : "0000";
            const std::string socialId = playerId;

            json tokens = {
                {"pragmaGameToken",   BuildJwt("GAME",   playerId, socialId, display, disc)},
                {"pragmaSocialToken", BuildJwt("SOCIAL", playerId, socialId, display, disc)}
            };

            json out = { {"pragmaTokens", tokens} };
            return reply(http::status::ok, out.dump());
        }

        return reply(http::status::not_found, R"({"error":"no route"})");
    }
    catch (const std::exception& e) {
        spdlog::error("auth 500: {}", e.what());
        return reply(http::status::internal_server_error, R"({"error":"internal server error"})");
    }
}

std::string AuthenticateHandler::CreatePlayerFromSteam(const std::string& steam64, const std::string& displayName) {
    const std::string uuid = PlayerUuidFromSteam64(steam64);

    ProfileData pd;
    pd.set_playerid(uuid);
    auto* dn = pd.mutable_displayname();
    dn->set_displayname(displayName.empty() ? "Player" : displayName);
    char disc[5];
    std::mt19937 rng{ std::random_device{}() };
    std::snprintf(disc, 5, "%04d", std::uniform_int_distribution<int>(0, 9999)(rng));
    dn->set_discriminator(disc);
    PlayerDatabase::Get().SetField(FieldKey::PROFILE_DATA, &pd, uuid);

    return uuid;
}

static std::string b64url_json(const nlohmann::json& j) {
    const std::string s = j.dump();
    static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out;
    out.reserve(((s.size() + 2) / 3) * 4);
    int val = 0, valb = -6;
    for (unsigned char c : s) {
        val = (val << 8) + c; valb += 8;
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
        {"kid", "d3JtOq6jy3_HquwTsrzt81wh3BLiA-4f-qM8mj-0-YQ="},
        {"alg", "RS256"},
        {"typ", "JWT"}
    };

    const std::string jti = boost::uuids::to_string(boost::uuids::random_generator()());
    nlohmann::json payload = {
        {"iss", "pragma"},
        {"sub", backendType == "GAME" ? playerId : socialId},
        {"iat", now},
        {"exp", exp},
        {"jti", jti},
        {"sessionType", "PLAYER"},
        {"backendType", backendType},
        {"displayName", displayName},
        {"discriminator", discriminator},
        {"pragmaSocialId", socialId},
        {"idProvider", "STEAM"},
        {"extSessionInfo", R"({"permissions":0,"accountTags":["canary"]})"},
        {"expiresInMillis", "86400000"},
        {"refreshInMillis", "36203000"},
        {"pragmaPlayerId", playerId}
    };

    if (backendType == "GAME") {
        payload["gameShardId"] = "00000000-0000-0000-0000-000000000001";
    }

    const std::string signing_input = b64url_json(header) + "." + b64url_json(payload);
    const std::string sig_b64url = sign_rs256_b64url(signing_input);
    return signing_input + "." + sig_b64url;
}