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

    static std::string Base64UrlEncode(const std::string &input);
    static std::string HmacSha256(const std::string& key, const std::string& data);
};