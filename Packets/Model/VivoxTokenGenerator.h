#pragma once
#include <string>

class VivoxTokenGenerator {
    public:

    static std::string Generate(
        const std::string& secretKey,
        const std::string& issuer,
        const std::string& domain,
        const std::string& subject,
        const std::string& action,
        const std::string& channel = "");

    private:

    static std::string base64url_encode(const std::string& in);
    static std::string hmac_sha256_b64url(const std::string& key, const std::string& msg);
};