//
// Created by astro on 27/09/2025.
//

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>
#include <thread>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

// todo maybe split the json and routing later
//  right now we keep it here for speed

// this is the static response for /v1/info
// we might have to split game and social onto different ports because some requests are the same
static auto INFO_JSON =
R"(
{
    "name":"mtn.live.game",
    "platformVersion":"0.0.339-0.0.101+3079-bc13023",
    "backendMode":"PRODUCTION",
    "authenticateBackend":{
        "host":"127.0.0.1",
        "port":7777,
        "protocol":"http",
        "scheme":"http"
    },
    "socialBackend":{
        "host":"127.0.0.1",
        "port":7777,
        "protocol":"http",
        "webSocketProtocol":"ws"
    },
    "gameBackend":{
        "host":"127.0.0.1",
        "port":7777,
        "protocol":"http",
        "webSocketProtocol":"ws"
    },
    "gameShardId":"00000000-0000-0000-0000-000000000001"
}
)";

// health payload for /v1/spectre/healthcheck-status
// if the game probes health, this tells it we are fine
static auto HEALTH_JSON =
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
static auto QUEUE_JSON =
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
static auto AUTH_JSON =
    R"(
{
    "pragmaTokens": {
        "pragmaGameToken": "eyJraWQiOiJkM0p0T3E2ankzX0hxdXdUc3J6dDgxd2gzQkxpQS00Zi1xTThtai0wLVlRPSIsImFsZyI6IlJTMjU2IiwidHlwIjoiSldUIn0.eyJpc3MiOiJwcmFnbWEiLCJzdWIiOiIyNTBkNmJlYS00ZjBkLTRkZWQtOTZjYy02ZTQwZWM4MGFmYWIiLCJpYXQiOjE3NDA1MDc0MjgsImV4cCI6MTc0MDU5MzgyOCwianRpIjoiZDFkZDU3ZmQtYTAxNy00ZDY1LTk2YzgtNDY3MGQwZWJhMjk5Iiwic2Vzc2lvblR5cGUiOiJQTEFZRVIiLCJyZWZyZXNoSW5NaWxsaXMiOiIzNTk5NDAwMCIsImV4cGlyZXNJbk1pbGxpcyI6Ijg2NDAwMDAwIiwiYmFja2VuZFR5cGUiOiJHQU1FIiwiZGlzcGxheU5hbWUiOiJTYW50YWlHRyIsImRpc2NyaW1pbmF0b3IiOiIzMzYxIiwicHJhZ21hU29jaWFsSWQiOiIyYzRlZWYxMS1kNTNlLTQ4YzItYTgwZi1hM2Y4MjM0ZTYxZjciLCJpZFByb3ZpZGVyIjoiU1RFQU0iLCJleHRTZXNzaW9uSW5mbyI6IntcInBlcm1pc3Npb25zXCI6MCxcImFjY291bnRUYWdzXCI6W119IiwicHJhZ21hUGxheWVySWQiOiIyNTBkNmJlYS00ZjBkLTRkZWQtOTZjYy02ZTQwZWM4MGFmYWIiLCJnYW1lU2hhcmRJZCI6IjAwMDAwMDAwLTAwMDAtMDAwMC0wMDAwLTAwMDAwMDAwMDAwMSJ9.dE-25OZkJJGIqh5Y3PxUynmSUvw2EHWJFG2DfrodH7TRrPHESfuhsWIbqF1VFPeqpFIlOgvpvg7QpRmrFKWuS0LQVwbRlhAUUa7r7O_VV8JLb4H4E6Ab0RNMCHB6LKmd5ULIXj_Cx8n7ZMtI3xdsdwewLdSySF4_I_x3o2YJ0bypgG3ABmh_j_KhM_WKLUMMK5AFVw_l5Z3t4YNorptBIMVCyQHCOZlv47fsIytWhsdkFkX-T-9XidSd9HKdSTV1TYUPdy0OIBWKITunbzzsL_hG8I5yWN1O45gzRXa4ZBEkJ2Dd5vIcy7qz9FuyndbzPmDWhAVloyPwETWAdyrAA1rrHou1HzoKdWsEwwCrsH6iQROR4bBJAdHkdE8n9bOVPdUrqA0ox0Z5VoUE3mZypYT_tybuX4nSZ0IT5Xky3O5aDZQ-qY8hpL1sCya6_spPHdOMHf9ZDSpopNsdpIDQDr3hXi821wvANWKen3A7dAfy4FYwqdT_brR2VwelHprul2ldKKeeKhR_CM9Y9i9g3SengNnc7JqOqnd4DHnzv4xXTKUzlClfvWJLJuCM0valCBuX0vjgJccbIbXl_Kel6mcG1ZEdZXlHba-gashSVNRE0-wJaP3-spuYU4cJCt6ajfKEvf0O-Ebc9MSApDQNCPHM5UEurZ_meoTaY2ywmEc",
        "pragmaSocialToken": "eyJraWQiOiJkM0p0T3E2ankzX0hxdXdUc3J6dDgxd2gzQkxpQS00Zi1xTThtai0wLVlRPSIsImFsZyI6IlJTMjU2IiwidHlwIjoiSldUIn0.eyJpc3MiOiJwcmFnbWEiLCJzdWIiOiIyYzRlZWYxMS1kNTNlLTQ4YzItYTgwZi1hM2Y4MjM0ZTYxZjciLCJpYXQiOjE3NDA1MDc0MjgsImV4cCI6MTc0MDU5MzgyOCwianRpIjoiODg2MzZhMjEtOTU2NC00OTNjLTllMGQtZjI2MjQ0ZjliYmExIiwic2Vzc2lvblR5cGUiOiJQTEFZRVIiLCJyZWZyZXNoSW5NaWxsaXMiOiIzNTgyODAwMCIsImV4cGlyZXNJbk1pbGxpcyI6Ijg2NDAwMDAwIiwiYmFja2VuZFR5cGUiOiJTT0NJQUwiLCJkaXNwbGF5TmFtZSI6IlNhbnRhaUdHIiwiZGlzY3JpbWluYXRvciI6IjMzNjEiLCJwcmFnbWFTb2NpYWxJZCI6IjJjNGVlZjExLWQ1M2UtNDhjMi1hODBmLWEzZjgyMzRlNjFmNyIsImlkUHJvdmlkZXIiOiJTVEVBTSIsImV4dFNlc3Npb25JbmZvIjoie1wicGVybWlzc2lvbnNcIjowLFwiYWNjb3VudFRhZ3NcIjpbXX0ifQ.UEyNYLjBTaV0TEB4q-02C8b0xqsVeOkn8KR6g2D2sJLyW5h4cmkFjVwmjyk0hT1pGSWO9XqAO9vPgmIliS-SO1nunzx7NHatgdcn3PsK1XpchPZum7TC7pTUEqryMRA3W7AfllX2TjhkSTSZF5pFmK7DDoz8icymjWxLKTcNxV9pJWn8rBZjD-yLo3-b3JsFtNtwKIdh4sdWY_0w3R1geTC3PTHt6FniRsxilEYBQwnSy6NTELLvaEI1dMH0YlrhXOE5r1CGuug9DcbVySGueZceNccptUrHBN56SbMiaBuAVcXhWsjVAtoTmxRDREe8DBX5PupbEYSSB8XbxyxnvWgYetHB2ULvB7tz6HHkVKQzMWi6mWOEGyOne_rRGfqbqNEh3ur7bkkC-3NmcYKIaUR88l88yh_g4bY5qfjKXOUknWYBa67G5Qop8nVI0S8DFpZILpCOgWWkIQQXpqGuxrJc7BeLbQoppCUeT-IpDS5dJ25Cy7AbeoKdF7jBqEhn2nOHSyVc36avADFWxko8hn82U6N0uBwS8vq23XBagmQh3ntpk0CV_isJ5SadSpc8oaTRtJA8uYly3h3taOT_IV9G585DbJu--7mbX6QUZCNCO_lsSsqTIjC-hz-GEbtUbb2IciiGR8v46VsT7k42o4UjSWNsY5xc1l696Ep6DqA"
    }
}
)";


// maybe load distribution probe or some shit i still dont know what this does
static auto GATEWAY_JSON = R"({"gateway":"3"})"; //todo split game and social gateways

// tiny router for http
// reads path from the request and fills the response with a json body
void handle_http(http::request<http::string_body> const& req,
                 http::response<http::string_body>& res) {
    // mirror request http version and tell the client we are json
    res.version(req.version());
    res.set(http::field::server, "FakePragma");
    res.set(http::field::content_type, "application/json");
    res.result(http::status::ok);

    auto target = std::string(req.target());
    // note the find(..)==0 check means prefix match, so query strings are fine
    if (target.find("/v1/info") == 0) {
        res.body() = INFO_JSON;
    } else if (target.find("/v1/spectre/healthcheck-status") == 0) {
        res.body() = HEALTH_JSON;
    } else if (target.find("/v1/loginqueue/getinqueuev1") == 0) {
        res.body() = QUEUE_JSON;
    } else if (target.find("/v1/account/authenticateorcreatev2") == 0) {
        res.body() = AUTH_JSON;
    } else if (target.find("/v1/gateway") == 0) {
        res.body() = GATEWAY_JSON;
    } else if (target.find("/v1/types") == 0) {
        // todo make this dynamic because its not compiling with all the types
        res.body() = R"({"AccountRpc.AddAccountTagsPartnerV1Request":13})";
    } else {
        // default not found keeps the client happy with a valid http response
        res.result(http::status::not_found);
        res.body() = "{}";
    }

    // beast will set content length etc based on body now
    res.prepare_payload();
}

// this handles one tcp connection
// either it upgrades to websocket and we loop forever
// or its plain http and we answer once and we are done
void session(tcp::socket sock) {
    try {
        beast::flat_buffer buffer; // http parser scratch space
        http::request<http::string_body> req;

        // blocking read
        http::read(sock, buffer, req);

        // detect websocket upgrade and switch protocols if requested
        if (websocket::is_upgrade(req)) {
            websocket::stream<tcp::socket> ws(std::move(sock));
            ws.accept(req); // handshake done, we are now speaking ws

            // basic echo loop so clients have something to talk to
            for (;;) {
                beast::flat_buffer wsbuf;
                ws.read(wsbuf); // this blocks until a message arrives or the peer closes
                auto msg = beast::buffers_to_string(wsbuf.data());
                std::cout << "[ws] got: " << msg << "\n";

                // send text mode json back, not mirroring payload right now
                ws.text(true);
                ws.write(asio::buffer(R"({"echo":"ok"})"));
            }
        }
        // plain http path, build a response and write it
        http::response<http::string_body> res;
        handle_http(req, res);
        http::write(sock, res);
        // no keep alive loop here. if you want it, wrap read and write in a while loop
    } catch (std::exception& e) {
        // any parse error, socket close, or ws close lands here
        std::cerr << "session error: " << e.what() << "\n";
    }
}

// the main accept loop
// binds to 127.0.0.1:7777, accepts a connection, spins a thread, repeat
int main() {
    std::cout << "running...\n";
    try {
        asio::io_context ioc; // we use sync ops but asio still wants an io_context around
        tcp::acceptor acc(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 7777));

        // accept loop forever. each client gets one detached thread
        // if you want to shut down clean, do not detach, keep thread handles
        for (;;) {
            tcp::socket sock(ioc);
            acc.accept(sock); // blocks until a client connects
            std::thread(&session, std::move(sock)).detach();
        }
    } catch (std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
    }
    return 0;
}