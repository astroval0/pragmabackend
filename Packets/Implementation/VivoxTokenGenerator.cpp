#include <VivoxTokenGenerator.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <ctime>
#include <sstream>
#include <random>
//#include "spdlog/spdlog.h"

static const std::string B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static unsigned int global_vxi = 0;

std::string VivoxTokenGenerator::base64url_encode(const std::string& in) {
    std::string out;
    int val = 0, valb = -6;

    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(B64[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        out.push_back(B64[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    return out;
}

std::string VivoxTokenGenerator::hmac_sha256_b64url(const std::string& key, const std::string& msg) {
    unsigned char hash[32];
    unsigned int len = 32;

    HMAC(
        EVP_sha256(),
        key.data(),
        static_cast<int>(key.size()),
        reinterpret_cast<const unsigned char*>(msg.data()),
        msg.size(),
        hash,
        &len
    );

    return base64url_encode(std::string(reinterpret_cast<char*>(hash), len));
}

std::string VivoxTokenGenerator::Generate(
    const std::string& secretKey,
    const std::string& issuer,
    const std::string& domain,
    const std::string& subject,
    const std::string& action,
    const std::string& channel
) {
    const int exp = static_cast<int>(std::time(nullptr) + 100);
    const int vxi = ++global_vxi;

    const std::string from =
        "sip:." + issuer + "." + subject + ".@" + domain;

    std::stringstream payload;

    payload << "{";
    payload << "\"iss\":\"" << issuer << "\",";
    payload << "\"exp\":" << exp << ",";
    payload << "\"vxa\":\"" << action << "\",";
    payload << "\"vxi\":" << vxi << ",";
    payload << "\"f\":\"" << from << "\"";

    if (action == "join") {
        payload << ",\"t\":\"sip:confctl-g-"
                << issuer << "." << channel << "@"
                << domain << "\"";
    }

    payload << "}";


    const std::string header = "e30"; // empty header \ {}
    const std::string payload_b64 = base64url_encode(payload.str());

    const std::string signing_input = header + "." + payload_b64;
    const std::string signature = hmac_sha256_b64url(secretKey, signing_input);

    //spdlog::info("Generated Token {}", signing_input + "." + signature);

    return signing_input + "." + signature;
}