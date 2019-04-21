//
// Created by Subhagato Dutta on 9/9/17.
//

#ifndef CRYPTOTRADER_APIKEY_H
#define CRYPTOTRADER_APIKEY_H

#include <nlohmann/json.hpp>

using namespace nlohmann;

class APIKey {
  std::string m_key;
  std::string m_secret;
  std::string m_passphrase;

 public:
  APIKey() : m_key(""), m_secret(""), m_passphrase(""){};
  APIKey(const std::string key, const std::string secret, const std::string passphrase = "")
      : m_key(key), m_secret(secret), m_passphrase(passphrase){};
  APIKey(json credentials) {
    if (credentials.find("key") != credentials.end()) m_key = credentials["key"].get<std::string>();
    if (credentials.find("secret") != credentials.end()) m_secret = credentials["secret"].get<std::string>();
    if (credentials.find("passphrase") != credentials.end())
      m_passphrase = credentials["passphrase"].get<std::string>();
  }

  void setKey(const std::string key) {
    m_key = key;
  }

  void setSecret(const std::string secret) {
    m_secret = secret;
  }

  const std::string getKey() const {
    return m_key;
  }

  const std::string getSecret() const {
    return m_secret;
  }

  const std::string getPassphrase() const {
    return m_passphrase;
  }

  void setPassphrase(const std::string passphrase) {
    m_passphrase = passphrase;
  }
};

#endif  // CRYPTOTRADER_APIKEY_H
