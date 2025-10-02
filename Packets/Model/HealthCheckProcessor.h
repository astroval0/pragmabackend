#include <PacketProcessor.h>

class HealthCheckProcessor : public HTTPPacketProcessor {
private:
	bool m_hasStatus;
public:
	HealthCheckProcessor(std::string route, bool hasStatus) : HTTPPacketProcessor(route), m_hasStatus(hasStatus) {};

	void Process(http::request<http::string_body> const& req, tls_stream& sock) override;
};