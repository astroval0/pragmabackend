#include <VivoxTokenGenerator.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <chrono>
#include <random>
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
//todo clean up this ugly mess and fix it not connecting to vivox
std::string VivoxTokenGenerator::Base64UrlEncode(const std::string& input) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    const char* bytes_to_encode = input.c_str();
    size_t in_len = input.size();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        int j = 0;
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++) ret += base64_chars[char_array_4[j]];
    }

    size_t len = ret.length();
    for (size_t k = 0; k < len; ++k) {
        if (ret[k] == '+') ret[k] = '-';
        else if (ret[k] == '/') ret[k] = '_';
    }

    while (!ret.empty() && ret.back() == '=') {
        ret.pop_back();
    }
    return ret;
}

std::string VivoxTokenGenerator::HmacSha256(const std::string& key, const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int len;

    HMAC(EVP_sha256(), key.c_str(), key.length(),
         reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
         hash, &len);

    return std::string(reinterpret_cast<char*>(hash), len);
}

std::string VivoxTokenGenerator::Generate(const std::string& secretKey, const std::string& issuer, const std::string& domain, const std::string& subject, const std::string& action, const std::string& channel) {
    std::string headerJson = "{}";
    std::string headerEnc = Base64UrlEncode(headerJson);

    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + 90;

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(100000, 999999);

    int vxi = dist(rng);
    std::string fromUri = "sip:." + issuer + "." + subject + ".@" + domain;

    std::string toUri;
    if (action == "join" && !channel.empty()) {
        //sip:confctl-g-{issuer}.{channel}@{domain}
        toUri = "sip:confctl-g-" + issuer + "." + channel + "@" + domain;
    }

    nlohmann::json payload;
    payload["iss"] = issuer;
    payload["exp"] = exp;
    payload["vxa"] = action;
    payload["vxi"] = vxi;
    payload["f"] = fromUri;
    payload["sub"] = subject;
    payload["sub"] = "";

    if (action == "join") {
        payload["t"] = toUri;
    } else {
        payload["t"] = nullptr;
    }

    std::string payloadEnc = Base64UrlEncode(payload.dump());
    std::string dataToSign = headerEnc + "." + payloadEnc;
    std::string signature = HmacSha256(secretKey, dataToSign);
    std::string signatureEnc = Base64UrlEncode(signature);

    return dataToSign + "." + signatureEnc;
}