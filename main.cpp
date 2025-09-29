//
// Created by astro on 27/09/2025.
//

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <filesystem>
#include <Registry.h>
#include <PacketProcessor.h>
#include <StaticResponseProcessor.h>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/logger.h>
#include <ctime>
#include <memory>
#include <SpectreWebsocket.h>
#include <SpectreWebsocketRequest.h>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
namespace ssl = asio::ssl;

static std::shared_ptr<spdlog::logger> logger;

void SetupLogger() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/app.log", true);

    // Optional: Customize sink formats
    console_sink->set_pattern("[%T] [%^%l%$] %v");
    file_sink->set_pattern("[%Y-%m-%d %T] [%l] %v");

    // Combine sinks into one logger
    std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
    logger = std::make_shared<spdlog::logger>("pragma", sinks.begin(), sinks.end());

    // Register and use the logger
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
}

// todo maybe split the json and routing later
//  right now we keep it here for speed

// this is the static response for /v1/info
// we might have to split game and social onto different ports because some requests are the same
static constexpr auto INFO_JSON =
R"(
{
    "name":"mtn.live.game",
    "platformVersion":"0.0.339-0.0.101+3079-bc13023",
    "backendMode":"PRODUCTION",
    "authenticateBackend":{
        "host":"127.0.0.1",
        "port":7777,
        "protocol":"https",
        "scheme":"https"
    },
    "socialBackend":{
        "host":"127.0.0.1",
        "port":7777,
        "protocol":"https",
        "webSocketProtocol":"wss"
    },
    "gameBackend":{
        "host":"127.0.0.1",
        "port":7777,
        "protocol":"https",
        "webSocketProtocol":"wss"
    },
    "gameShardId":"00000000-0000-0000-0000-000000000001"
}
)";

// health payload for /v1/spectre/healthcheck-status
// if the game probes health, this tells it we are fine
static constexpr auto HEALTH_JSON =
    R"(
{
    "health": {
        "isHealthy": true,
        "version": "0.0.339-0.0.101-67406b2",
        "platformVersion": "0.0.339-0.0.101+4402-67406b2",
        "portalVersion": "0.0.339-0.0.101+4402-67406b2",
        "contentVersion": "20250225.season-1-content.0002-a0b14dd",
        "configVersion": "20250225.season-1.0004-8ecb200",
        "launchTimeUnixMillis": "1740497952051"
    },
    "status": "UNDEFINED"
}
)";

// these jwt strings are ones from our captures.
// the game might only check that they look valid, not that they are signed by anyone
// if it starts validating them, we will have to make real ones
static constexpr auto QUEUE_JSON =
    R"(
{
    "token": "eyJraWQiOiJkM0p0T3E2ankzX0hxdXdUc3J6dDgxd2gzQkxpQS00Zi1xTThtai0wLVlRPSIsImFsZyI6IlJTMjU2IiwidHlwIjoiSldUIn0.eyJpc3MiOiJwcmFnbWEiLCJzdWIiOiIxMTYyOCIsImlhdCI6MTc0MDUwNzQyNywiZXhwIjoxNzQwNTkzODI3LCJqdGkiOiIyYjFiMzc5Yi1jZmQwLTRkMzAtYWE4NS02OWMyZWIyMWE4MWEiLCJ0aWNrZXROdW1iZXIiOiIxMTYyOCIsImlzQWxsb3dlZEluIjoidHJ1ZSJ9.PJp5MNXz2_xvCkq_XjzZeui1MvS9ylDLDgeLkJiv9jp_FVnTI9LISMtajHcef-7JehNs5sQC6P_Gpmb6JuVdD4k7HUX7a9IAgM8HKAagnfgmymn02SSpL7Mfz9wbH8FgOYU2ylKG_ExIW_aSG5HK588_waNeSydygwX2zRoSf8ZYZzbUHmMsZcG2iXpDq_Peejbt6Cgep9lsyNE5L5ZZzil9_KVu3FaEojcrI7tiPpHX7wi2K_J78rxmg2weUreowhv0VJA-YGqtOUlqFl7Ep8VGi-IrJdAf4gLeiVZMQoktc_g5tD9FgXzEAH_aDoBqGgoqnbKLcWLRiT1TAYGgXtCfw15Efh_ta-h4IIOI-DAnhJ1ujapd80Z87Wo6h7SpBaOitaI-bjBPkqDQGe2JooUNCrki848vPrfu0IQW00vawUtLX6LaS_aAEs0L2Vjxyebk1X37E9KwTDoxQGdmurutcnvSmVXOoO4P8F6o4oGx-A9d6HgFJl5rRie2LrWSJHlmcFm5_IKYw7okHwBh63Cx3mhUevji5SkEGj3gbwlBURjeEXpOm0qr-ECeKdmagbi_ipiiQB8m8FNwAbx9Z-Sl3nbJ-kS3QtPZrFHqxf91sgFY16H6sn1ruhna-ZygG5cYKf4JWbEcmLrSmdQ_xIBODjWDcatvNKGrv7Cx_Ng",
    "positionInQueue": 0,
    "eta": "0",
    "pollNextDuration": 1000,
    "isAllowedIn": true
}
)";

// jwt tokens for auth route
static constexpr auto AUTH_JSON =
    R"(
{
    "pragmaTokens": {
        "pragmaGameToken": "eyJraWQiOiJkM0p0T3E2ankzX0hxdXdUc3J6dDgxd2gzQkxpQS00Zi1xTThtai0wLVlRPSIsImFsZyI6IlJTMjU2IiwidHlwIjoiSldUIn0.eyJpc3MiOiJwcmFnbWEiLCJzdWIiOiIyNTBkNmJlYS00ZjBkLTRkZWQtOTZjYy02ZTQwZWM4MGFmYWIiLCJpYXQiOjE3NDA1MDc0MjgsImV4cCI6MTc0MDU5MzgyOCwianRpIjoiZDFkZDU3ZmQtYTAxNy00ZDY1LTk2YzgtNDY3MGQwZWJhMjk5Iiwic2Vzc2lvblR5cGUiOiJQTEFZRVIiLCJyZWZyZXNoSW5NaWxsaXMiOiIzNTk5NDAwMCIsImV4cGlyZXNJbk1pbGxpcyI6Ijg2NDAwMDAwIiwiYmFja2VuZFR5cGUiOiJHQU1FIiwiZGlzcGxheU5hbWUiOiJTYW50YWlHRyIsImRpc2NyaW1pbmF0b3IiOiIzMzYxIiwicHJhZ21hU29jaWFsSWQiOiIyYzRlZWYxMS1kNTNlLTQ4YzItYTgwZi1hM2Y4MjM0ZTYxZjciLCJpZFByb3ZpZGVyIjoiU1RFQU0iLCJleHRTZXNzaW9uSW5mbyI6IntcInBlcm1pc3Npb25zXCI6MCxcImFjY291bnRUYWdzXCI6W119IiwicHJhZ21hUGxheWVySWQiOiIyNTBkNmJlYS00ZjBkLTRkZWQtOTZjYy02ZTQwZWM4MGFmYWIiLCJnYW1lU2hhcmRJZCI6IjAwMDAwMDAwLTAwMDAtMDAwMC0wMDAwLTAwMDAwMDAwMDAwMSJ9.dE-25OZkJJGIqh5Y3PxUynmSUvw2EHWJFG2DfrodH7TRrPHESfuhsWIbqF1VFPeqpFIlOgvpvg7QpRmrFKWuS0LQVwbRlhAUUa7r7O_VV8JLb4H4E6Ab0RNMCHB6LKmd5ULIXj_Cx8n7ZMtI3xdsdwewLdSySF4_I_x3o2YJ0bypgG3ABmh_j_KhM_WKLUMMK5AFVw_l5Z3t4YNorptBIMVCyQHCOZlv47fsIytWhsdkFkX-T-9XidSd9HKdSTV1TYUPdy0OIBWKITunbzzsL_hG8I5yWN1O45gzRXa4ZBEkJ2Dd5vIcy7qz9FuyndbzPmDWhAVloyPwETWAdyrAA1rrHou1HzoKdWsEwwCrsH6iQROR4bBJAdHkdE8n9bOVPdUrqA0ox0Z5VoUE3mZypYT_tybuX4nSZ0IT5Xky3O5aDZQ-qY8hpL1sCya6_spPHdOMHf9ZDSpopNsdpIDQDr3hXi821wvANWKen3A7dAfy4FYwqdT_brR2VwelHprul2ldKKeeKhR_CM9Y9i9g3SengNnc7JqOqnd4DHnzv4xXTKUzlClfvWJLJuCM0valCBuX0vjgJccbIbXl_Kel6mcG1ZEdZXlHba-gashSVNRE0-wJaP3-spuYU4cJCt6ajfKEvf0O-Ebc9MSApDQNCPHM5UEurZ_meoTaY2ywmEc",
        "pragmaSocialToken": "eyJraWQiOiJkM0p0T3E2ankzX0hxdXdUc3J6dDgxd2gzQkxpQS00Zi1xTThtai0wLVlRPSIsImFsZyI6IlJTMjU2IiwidHlwIjoiSldUIn0.eyJpc3MiOiJwcmFnbWEiLCJzdWIiOiIyYzRlZWYxMS1kNTNlLTQ4YzItYTgwZi1hM2Y4MjM0ZTYxZjciLCJpYXQiOjE3NDA1MDc0MjgsImV4cCI6MTc0MDU5MzgyOCwianRpIjoiODg2MzZhMjEtOTU2NC00OTNjLTllMGQtZjI2MjQ0ZjliYmExIiwic2Vzc2lvblR5cGUiOiJQTEFZRVIiLCJyZWZyZXNoSW5NaWxsaXMiOiIzNTgyODAwMCIsImV4cGlyZXNJbk1pbGxpcyI6Ijg2NDAwMDAwIiwiYmFja2VuZFR5cGUiOiJTT0NJQUwiLCJkaXNwbGF5TmFtZSI6IlNhbnRhaUdHIiwiZGlzY3JpbWluYXRvciI6IjMzNjEiLCJwcmFnbWFTb2NpYWxJZCI6IjJjNGVlZjExLWQ1M2UtNDhjMi1hODBmLWEzZjgyMzRlNjFmNyIsImlkUHJvdmlkZXIiOiJTVEVBTSIsImV4dFNlc3Npb25JbmZvIjoie1wicGVybWlzc2lvbnNcIjowLFwiYWNjb3VudFRhZ3NcIjpbXX0ifQ.UEyNYLjBTaV0TEB4q-02C8b0xqsVeOkn8KR6g2D2sJLyW5h4cmkFjVwmjyk0hT1pGSWO9XqAO9vPgmIliS-SO1nunzx7NHatgdcn3PsK1XpchPZum7TC7pTUEqryMRA3W7AfllX2TjhkSTSZF5pFmK7DDoz8icymjWxLKTcNxV9pJWn8rBZjD-yLo3-b3JsFtNtwKIdh4sdWY_0w3R1geTC3PTHt6FniRsxilEYBQwnSy6NTELLvaEI1dMH0YlrhXOE5r1CGuug9DcbVySGueZceNccptUrHBN56SbMiaBuAVcXhWsjVAtoTmxRDREe8DBX5PupbEYSSB8XbxyxnvWgYetHB2ULvB7tz6HHkVKQzMWi6mWOEGyOne_rRGfqbqNEh3ur7bkkC-3NmcYKIaUR88l88yh_g4bY5qfjKXOUknWYBa67G5Qop8nVI0S8DFpZILpCOgWWkIQQXpqGuxrJc7BeLbQoppCUeT-IpDS5dJ25Cy7AbeoKdF7jBqEhn2nOHSyVc36avADFWxko8hn82U6N0uBwS8vq23XBagmQh3ntpk0CV_isJ5SadSpc8oaTRtJA8uYly3h3taOT_IV9G585DbJu--7mbX6QUZCNCO_lsSsqTIjC-hz-GEbtUbb2IciiGR8v46VsT7k42o4UjSWNsY5xc1l696Ep6DqA"
    }
}
)";


// maybe load distribution probe or some shit i still dont know what this does
static constexpr auto GATEWAY_JSON = R"({"gateway":"3"})"; //todo split game and social gateways

static std::string stripQueryParams(const std::string& url) {
    size_t pos = url.find('?');
    if (pos != std::string::npos) {
        return url.substr(0, pos);
    }
    return url;
}

static std::string cert_path(const char* f){ return (std::filesystem::path(CERT_DIR)/f).string(); }

// load TLS certs / keys
// we terminate TLS here so the game can speak https / wss to us directly
static void ConfigureTlsContext(ssl::context& ctx) {
    ctx.set_options(
        ssl::context::default_workarounds
        | ssl::context::no_sslv2
        | ssl::context::no_sslv3
        | ssl::context::no_tlsv1
        | ssl::context::no_tlsv1_1
        | ssl::context::single_dh_use
    );


    ctx.use_certificate_chain_file(cert_path("server.crt"));
    ctx.use_private_key_file(cert_path("server.key"), ssl::context::file_format::pem);
}

void session(tcp::socket sock, ssl::context& tls_ctx) {
    try {
        using tls_stream = ssl::stream<tcp::socket>; // tls wrapper around a plain tcp socket
        websocket::stream<tls_stream> wss{std::move(sock), tls_ctx}; // build tls inside ws so we don't copy /move ssl streams

        http::request<http::string_body> req;
        beast::flat_buffer buffer;
        http::read(wss.next_layer().next_layer(), buffer, req);

        wss.next_layer().handshake(ssl::stream_base::server); // tls server handshake on the underlying stream

        // detect websocket upgrade and switch protocols if requested
        if (websocket::is_upgrade(req)) {
            websocket::stream<tcp::socket> rawSock(std::move(sock));
            SpectreWebsocket sock(rawSock);
            logger->info("upgraded connection with " + rawSock.next_layer().remote_endpoint().address().to_string() + ":" + std::to_string(rawSock.next_layer().remote_endpoint().port()) + " to websocket");
            rawSock.accept(req); // handshake done, we are now speaking ws

            // basic echo loop so clients have something to talk to
            for (;;) {
                beast::flat_buffer wsbuf;
                rawSock.read(wsbuf); // this blocks until a message arrives or the peer closes
                SpectreWebsocketRequest req(sock, wsbuf);
                auto route = Registry::WEBSOCKET_ROUTES.find(req.GetRequestType());
                if (route == Registry::WEBSOCKET_ROUTES.end()) {
                    logger->warn("no packet processor found for WS requestType: " + req.GetRequestType().GetName());
                    continue;
                }
                route->second->Process(req, sock);
            }
        }

        auto target = stripQueryParams(std::string(req.target())); // remove ?query so routing is stable
        HTTPPacketProcessor* processor = Registry::HTTP_ROUTES[target];
        if (processor == nullptr) {
            logger->warn("missing a handler for http route " + target);
            // send a 404 if no processor found
            http::response<http::string_body> res;
            res.result(http::status::not_found);
            res.body() = "{}";
            res.prepare_payload();
            http::write(sock, res);
            return;
        }
        processor->Process(req, &sock);
    } catch (std::exception& e) {
        logger->error("session error: "); // any parse/handshake/io error ends up here
        logger->error(e.what());          // print the reason so we can fix it
    }
}

void RegisterHandlers() {
    logger->info("Registering handlers...");
    Registry::HTTP_ROUTES["/v1/info"] = new StaticResponseProcessorHTTP("/v1/info", asio::buffer(INFO_JSON, sizeof(INFO_JSON)));
    Registry::HTTP_ROUTES["/v1/spectre/healthcheck-status"] = new StaticResponseProcessorHTTP("/v1/spectre/healthcheck-status", asio::buffer(HEALTH_JSON, sizeof(HEALTH_JSON)));
    Registry::HTTP_ROUTES["/v1/loginqueue/getinqueuev1"] = new StaticResponseProcessorHTTP("/v1/loginqueue/getinqueuev1", asio::buffer(QUEUE_JSON, sizeof(QUEUE_JSON)));
    Registry::HTTP_ROUTES["/v1/account/authenticateorcreatev2"] = new StaticResponseProcessorHTTP("/v1/account/authenticateorcreatev2", asio::buffer(AUTH_JSON, sizeof(AUTH_JSON)));
    Registry::HTTP_ROUTES["/v1/gateway"] = new StaticResponseProcessorHTTP("/v1/gateway", asio::buffer(GATEWAY_JSON, sizeof(GATEWAY_JSON)));
}

// the main accept loop
// binds to 127.0.0.1:7777, accepts a connection, spins a thread, repeat
int main() {
    SetupLogger();
    logger->info("starting server...");
    RegisterHandlers();
    try {
        asio::io_context ioc; // we use sync ops but asio still wants an io_context around

        // tls setup. load cert / key
        ssl::context tls_ctx(ssl::context::tls_server);
        ConfigureTlsContext(tls_ctx);

        tcp::acceptor acc(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 7777));

        // accept loop forever. each client gets one detached thread
        // if we want to shut down clean, don't detach, keep thread handles
        for (;;) {
            tcp::socket sock(ioc);
            acc.accept(sock); // blocks until a client connects

            // each session owns its TLS handshake and stream
            std::thread([s = std::move(sock), &tls_ctx]() mutable {
                session(std::move(s), tls_ctx);
            }).detach();
        }
    } catch (std::exception& e) {
        logger->error("fatal exception: ");
        logger->error(e.what());
        return 1;
    }
    return 0;
}