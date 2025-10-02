#include <HealthCheckProcessor.h>
#include <chrono>

void HealthCheckProcessor::Process(http::request<http::string_body> const& req, tls_stream& sock) {
	json statuspayload = json::object();
    statuspayload["isHealthy"] = true;
    statuspayload["version"] = "0.0.339-0.0.101-67406b2";
    statuspayload["platformVersion"] = "0.0.339-0.0.101+4402-67406b2";
    statuspayload["portalVersion"] = "0.0.339-0.0.101+4402-67406b2";
    statuspayload["contentVersion"] = "20250225.season-1-content.0002-a0b14dd";
    statuspayload["configVersion"] = "20250225.season-1.0004-8ecb200";
    auto ts = std::chrono::system_clock::now();
    statuspayload["launchTimeUnixMillis"] = duration_cast<std::chrono::milliseconds>(ts.time_since_epoch()).count();
    http::response<http::string_body> res;
    res.version(20); // HTTP/2
    res.result(http::status::ok);
    res.set(http::field::content_type, "application/json; charset=UTF-8");
    res.set(http::field::vary, "Origin");
    if (m_hasStatus) {
        json rootpayload = json::object();
        rootpayload["status"] = "UNDEFINED";
        rootpayload["health"] = statuspayload;
        res.body() = rootpayload.dump();
    }
    else {
        res.body() = statuspayload.dump();
    }
    res.prepare_payload();
    http::write(sock, res);
}