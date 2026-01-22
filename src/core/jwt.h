/**
 * @file jwt.h
 * @brief Simple JWT (JSON Web Token) implementation
 */

#pragma once

#include <ctime>
#include <map>
#include <string>

namespace Tootega
{
namespace Core
{

/**
 * @brief Simple JWT handler for authentication
 */
class JWT
{
  public:
    /**
     * @brief JWT payload data
     */
    struct Payload
    {
        std::string sub;                           // Subject (user identifier)
        std::string name;                          // User name
        std::time_t iat;                           // Issued at
        std::time_t exp;                           // Expiration time
        std::map<std::string, std::string> claims; // Additional claims
    };

    /**
     * @brief Create a new JWT token
     * @param userId User identifier
     * @param userName User display name
     * @param expirationSeconds Token expiration in seconds (default 24 hours)
     * @return JWT token string
     */
    static std::string createToken(const std::string &userId, const std::string &userName,
                                   int expirationSeconds = 86400);

    /**
     * @brief Verify and decode a JWT token
     * @param token JWT token string
     * @param payload Output payload if valid
     * @return true if token is valid, false otherwise
     */
    static bool verifyToken(const std::string &token, Payload &payload);

    /**
     * @brief Check if a token is expired
     * @param token JWT token string
     * @return true if expired, false otherwise
     */
    static bool isExpired(const std::string &token);

    /**
     * @brief Get the secret key (for internal use)
     */
    static const std::string &getSecret();

    /**
     * @brief Set a custom secret key
     */
    static void setSecret(const std::string &secret);

  private:
    static std::string base64Encode(const std::string &input);
    static std::string base64Decode(const std::string &input);
    static std::string hmacSha256(const std::string &data, const std::string &key);
    static std::string createSignature(const std::string &header, const std::string &payload);

    static std::string s_secret;
};

} // namespace Core
} // namespace Tootega
