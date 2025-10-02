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
#include <PacketProcessor.h>
#include <StaticResponseProcessorHTTP.h>
#include <StaticResponseProcessorWS.h>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/logger.h>
#include <ctime>
#include <memory>
#include <SpectreWebsocket.h>
#include <SpectreWebsocketRequest.h>
#include <HeartbeatProcessor.h>
#include "StaticHTTPPackets.cpp"
#include "StaticWSPackets.cpp"
#include <HealthCheckProcessor.h>

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

static std::string stripQueryParams(const std::string& url) {
	size_t pos = url.find('?');
	if (pos != std::string::npos) {
		return url.substr(0, pos);
	}
	return url;
}

static std::string cert_path(const char* f) { return (std::filesystem::path(CERT_DIR) / f).string(); }

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
		websocket::stream<tls_stream> wss{ std::move(sock), tls_ctx }; // build tls inside ws so we don't copy /move ssl streams
		boost::system::error_code err;
		wss.next_layer().handshake(ssl::stream_base::server, err); // tls server handshake on the underlying stream
		if (err) {
			logger->error("TLS Handshake failure: " + err.message());
			return;
		}

		http::request<http::string_body> req;
		beast::flat_buffer buffer;
		http::read(wss.next_layer(), buffer, req);

		// detect websocket upgrade and switch protocols if requested
		if (websocket::is_upgrade(req)) {
			wss.accept(req); // complete ws handshake
			SpectreWebsocket sock(wss);
			logger->info("upgraded connection with " + wss.next_layer().next_layer().remote_endpoint().address().to_string() + ":" + std::to_string(wss.next_layer().next_layer().remote_endpoint().port()) + " to websocket");

			// basic echo loop so clients have something to talk to
			for (;;) {
				beast::flat_buffer wsbuf;
				wss.read(wsbuf); // this blocks until a message arrives or the peer closes
				SpectreWebsocketRequest req(sock, wsbuf);
				auto route = WebsocketPacketProcessor::GetProcessorForRpc(req.GetRequestType());
				if (route == nullptr) {
					logger->warn("no packet processor found for WS requestType: " + req.GetRequestType().GetName());
					continue;
				}
				route->Process(req, sock);
			}
		}

		auto target = stripQueryParams(std::string(req.target())); // remove ?query so routing is stable
		HTTPPacketProcessor* processor = HTTPPacketProcessor::GetProcessorForRoute(target);
		if (processor == nullptr) {
			logger->warn("missing a handler for http route " + target);
			// send a 404 if no processor found
			http::response<http::string_body> res;
			res.result(http::status::not_found);
			res.body() = "{}";
			res.prepare_payload();
			http::write(wss.next_layer(), res);
			return;
		}
		processor->Process(req, wss.next_layer());
	}
	catch (std::exception& e) {
		logger->error("session error: "); // any parse/handshake/io error ends up here
		logger->error(e.what());          // print the reason so we can fix it
	}
}

// the main accept loop
// binds to 127.0.0.1:7777, accepts a connection, spins a thread, repeat
int main() {
	SetupLogger();
	logger->info("starting server...");
	logger->info("Registering handlers...");
	RegisterStaticHTTPHandlers();
	RegisterStaticWSHandlers();
	new HeartbeatProcessor(SpectreRpcType("PlayerSessionRpc.HeartbeatV1Request"));
	new HealthCheckProcessor("/v1/spectre/healthcheck-status", true);
	new HealthCheckProcessor("/v1/healthcheck", false);
	logger->info("Finished registering handlers");
	try {
		asio::io_context ioc; // we use sync ops but asio still wants an io_context around

		// tls setup. load cert / key
		ssl::context tls_ctx(ssl::context::tls_server);
		ConfigureTlsContext(tls_ctx);

		tcp::acceptor acc(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 443));

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
	}
	catch (std::exception& e) {
		logger->error("fatal exception: ");
		logger->error(e.what());
		return 1;
	}
	return 0;
}