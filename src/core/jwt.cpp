/**
 * @file jwt.cpp
 * @brief Simple JWT implementation
 */

#include "jwt.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>

namespace Tootega
{
namespace Core
{

// Default secret key (should be changed in production)
std::string JWT::s_secret = "TootegaWebAPI_SecretKey_2026_ChangeInProduction!";

const std::string &JWT::getSecret()
{
    return s_secret;
}

void JWT::setSecret(const std::string &secret)
{
    s_secret = secret;
}

std::string JWT::base64Encode(const std::string &input)
{
    static const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encoded;
    encoded.reserve(((input.size() + 2) / 3) * 4);

    unsigned int val = 0;
    int valb = -6;

    for (unsigned char c : input)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            encoded.push_back(base64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6)
    {
        encoded.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    // URL-safe base64: replace + with -, / with _, and remove =
    std::replace(encoded.begin(), encoded.end(), '+', '-');
    std::replace(encoded.begin(), encoded.end(), '/', '_');
    // Remove padding
    encoded.erase(std::find(encoded.begin(), encoded.end(), '='), encoded.end());

    return encoded;
}

std::string JWT::base64Decode(const std::string &input)
{
    static const int base64Index[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 62, -1, 63, 52, 53, 54, 55,
        56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
        13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63, -1, 26, 27, 28, 29, 30, 31, 32,
        33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1};

    // Convert URL-safe base64 back to standard
    std::string paddedInput = input;
    std::replace(paddedInput.begin(), paddedInput.end(), '-', '+');
    std::replace(paddedInput.begin(), paddedInput.end(), '_', '/');

    // Add padding if necessary
    while (paddedInput.size() % 4 != 0)
    {
        paddedInput += '=';
    }

    std::string decoded;
    decoded.reserve((paddedInput.size() / 4) * 3);

    unsigned int val = 0;
    int valb = -8;

    for (char c : paddedInput)
    {
        if (c == '=')
            break;

        int idx = base64Index[static_cast<unsigned char>(c)];
        if (idx == -1)
            continue;

        val = (val << 6) + idx;
        valb += 6;

        if (valb >= 0)
        {
            decoded.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return decoded;
}

// Simple HMAC-SHA256 implementation (simplified for demonstration)
// In production, use a proper crypto library
std::string JWT::hmacSha256(const std::string &data, const std::string &key)
{
    // This is a simplified hash for demonstration
    // In production, use OpenSSL or similar
    unsigned int hash = 0x811c9dc5;        // FNV offset basis
    const unsigned int prime = 0x01000193; // FNV prime

    // Mix key into hash
    for (char c : key)
    {
        hash ^= static_cast<unsigned char>(c);
        hash *= prime;
    }

    // Mix data into hash
    for (char c : data)
    {
        hash ^= static_cast<unsigned char>(c);
        hash *= prime;
    }

    // Create a pseudo-signature by combining multiple hash rounds
    std::string result;
    result.reserve(32);

    for (int i = 0; i < 32; ++i)
    {
        hash ^= (hash >> 13);
        hash *= prime;
        hash ^= (hash >> 7);
        result.push_back(static_cast<char>(hash & 0xFF));
    }

    return result;
}

std::string JWT::createSignature(const std::string &header, const std::string &payload)
{
    std::string data = header + "." + payload;
    std::string signature = hmacSha256(data, s_secret);
    return base64Encode(signature);
}

std::string JWT::createToken(const std::string &userId, const std::string &userName, int expirationSeconds)
{
    std::time_t now = std::time(nullptr);
    std::time_t exp = now + expirationSeconds;

    // Header (always the same for HS256)
    std::string header = R"({"alg":"HS256","typ":"JWT"})";
    std::string encodedHeader = base64Encode(header);

    // Payload
    std::ostringstream payloadJson;
    payloadJson << "{";
    payloadJson << "\"sub\":\"" << userId << "\",";
    payloadJson << "\"name\":\"" << userName << "\",";
    payloadJson << "\"iat\":" << now << ",";
    payloadJson << "\"exp\":" << exp;
    payloadJson << "}";

    std::string encodedPayload = base64Encode(payloadJson.str());

    // Signature
    std::string signature = createSignature(encodedHeader, encodedPayload);

    // Combine all parts
    return encodedHeader + "." + encodedPayload + "." + signature;
}

bool JWT::verifyToken(const std::string &token, Payload &payload)
{
    // Split token into parts
    std::vector<std::string> parts;
    std::istringstream iss(token);
    std::string part;

    while (std::getline(iss, part, '.'))
    {
        parts.push_back(part);
    }

    if (parts.size() != 3)
    {
        return false;
    }

    // Verify signature
    std::string expectedSignature = createSignature(parts[0], parts[1]);
    if (parts[2] != expectedSignature)
    {
        return false;
    }

    // Decode payload
    std::string decodedPayload = base64Decode(parts[1]);

    // Parse payload (simple JSON parsing)
    auto extractValue = [&decodedPayload](const std::string &key) -> std::string {
        std::string searchKey = "\"" + key + "\":";
        size_t pos = decodedPayload.find(searchKey);
        if (pos == std::string::npos)
            return "";

        pos += searchKey.length();

        // Skip whitespace
        while (pos < decodedPayload.length() && (decodedPayload[pos] == ' ' || decodedPayload[pos] == '\t'))
            pos++;

        if (pos >= decodedPayload.length())
            return "";

        if (decodedPayload[pos] == '"')
        {
            // String value
            pos++;
            size_t end = decodedPayload.find('"', pos);
            if (end == std::string::npos)
                return "";
            return decodedPayload.substr(pos, end - pos);
        }
        else
        {
            // Number value
            size_t end = pos;
            while (end < decodedPayload.length() && (std::isdigit(decodedPayload[end]) || decodedPayload[end] == '-'))
                end++;
            return decodedPayload.substr(pos, end - pos);
        }
    };

    payload.sub = extractValue("sub");
    payload.name = extractValue("name");

    std::string iatStr = extractValue("iat");
    std::string expStr = extractValue("exp");

    payload.iat = iatStr.empty() ? 0 : std::stoll(iatStr);
    payload.exp = expStr.empty() ? 0 : std::stoll(expStr);

    // Check expiration
    std::time_t now = std::time(nullptr);
    if (payload.exp > 0 && payload.exp < now)
    {
        return false; // Token expired
    }

    return true;
}

bool JWT::isExpired(const std::string &token)
{
    Payload payload;
    if (!verifyToken(token, payload))
    {
        return true;
    }

    std::time_t now = std::time(nullptr);
    return payload.exp > 0 && payload.exp < now;
}

} // namespace Core
} // namespace Tootega
